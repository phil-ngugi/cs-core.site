# Kubernetes Networking — The Complete Chain

From pod creation to a fully-traced Service call, at the exact mechanism level.

---

## Part 1 — Pod creation and network setup

**Prerequisite state (already in place before this pod exists):** the
kube-controller-manager's node-ipam controller has allocated Node B a pod
subnet (`10.244.2.0/24`), written to `Node.spec.podCIDR`. Flannel's DaemonSet
pod (`flanneld`) on Node B has already read that value, written it to
`/run/flannel/subnet.env`, configured the local `cni0` bridge with address
`10.244.2.1/24`, and installed VXLAN routes for every *other* node's subnet.

1. **API server** persists the new Pod object to etcd. `.status.podIP` is empty.
2. **Scheduler** selects Node B, writes `.spec.nodeName = nodeB` back via the API server.
3. **kubelet** on Node B (watching the API server for pods bound to it) sees the pod.
4. kubelet calls **containerd** over the **CRI**: `RunPodSandbox`.
5. containerd asks **runc** to create the **pause container** — namespaces
   (network, IPC, UTS shared pod-wide; PID and mount kept private per
   container) plus cgroups. The network namespace is empty at this point:
   no interface, no IP, just loopback.
6. containerd reads the CNI config in `/etc/cni/net.d/` (written by flanneld)
   and **exec's the CNI plugin** against the pause container's network
   namespace, passing the netns path and pod identity via env vars and stdin
   — **not** an IP; the IP doesn't exist yet.
7. The Flannel CNI plugin delegates to the reference **`bridge`** + **`host-local`**
   plugins:
   - `host-local` (IPAM) allocates the lowest free IP in `10.244.2.0/24` —
     say `10.244.2.5` — recorded locally in `/var/lib/cni/networks/...`.
   - `bridge` creates a **veth pair**, moves one end into the pod's netns
     (becomes its `eth0`, assigned `10.244.2.5/24`), attaches the other end
     to the node's **`cni0`** bridge, and installs the pod's default route
     (`default via 10.244.2.1 dev eth0`).
   - The kernel **automatically** creates the connected route
     `10.244.2.0/24 dev cni0` on the node as a side effect of `cni0`
     having that address.
8. The CNI plugin prints the assigned IP as JSON to stdout. containerd reads
   it, includes it in `PodSandboxStatus`, returns it to the kubelet over CRI.
9. **kubelet writes `.status.podIP = 10.244.2.5`** to the API server — this
   is the kubelet reporting *observed* reality upward; it never fetched the
   IP from anywhere, it received it from the CNI it just invoked.
10. kubelet calls containerd `CreateContainer`/`StartContainer` for the app
    container(s), which **join** (via `setns()`) the pause container's
    existing net/IPC/UTS namespaces rather than creating their own.
11. Once the pod passes readiness, the **EndpointSlice controller** (watching
    Services + Pods) adds `10.244.2.5` to the EndpointSlice of any Service
    that selects this pod's labels.
12. **kube-proxy** on every node (watching EndpointSlices) updates its
    iptables `nat` table so that Service's ClusterIP DNAT rules now include
    `10.244.2.5` as a candidate backend.

Pod networking is now fully live: real IP, locally bridged, reachable
cross-node via Flannel's pre-installed routes, and — if selected by a
Service — a registered load-balancing target.

---

## Part 2 — DNS resolution (the layer before any packet is built)

A pod's `/etc/resolv.conf` is written by the kubelet at pod creation,
pointing at the cluster DNS ClusterIP (`--cluster-dns`, typically `10.96.0.10`):

```
nameserver 10.96.0.10
search default.svc.cluster.local svc.cluster.local cluster.local
ndots:5
```

1. The application calls `getaddrinfo("my-service")`. Because the name has
   fewer dots than `ndots:5`, the resolver appends the search domains and
   tries `my-service.default.svc.cluster.local` first.
2. The resolver sends a **DNS query** (UDP, port 53) to `10.96.0.10` — this
   is itself a Service call, so it goes through the **exact same DNAT
   sequence described in Part 3** to reach one of the CoreDNS pods.
3. **CoreDNS**, watching the API server for Services/EndpointSlices, is
   authoritative for `cluster.local` — it answers directly with the target
   Service's **ClusterIP** (e.g. `10.96.5.20`), not a pod IP. (For external
   names it doesn't own, it *forwards* to whatever upstream is in its own
   pod's `/etc/resolv.conf`, inherited from the node.)
4. The reply is relayed back through conntrack (reversing the DNAT that
   routed the query to that CoreDNS pod) and delivered to the app.
5. The application now has `10.96.5.20` and calls `connect()`.

The client never learns a pod IP through this process — it only ever holds
the ClusterIP, by design, so pods behind the Service remain interchangeable.

---

## Part 3 — Pod-to-pod communication via a Service (Pod-A → Pod-B)

Pod-A is on Node A (`10.244.1.5`); Pod-B is on Node B (`10.244.2.5`,
`10.244.2.0/24`, VNI on `flannel.1`). Node A's real IP: `192.168.1.10`.
Node B's real IP: `192.168.1.11`.

**Socket construction (before any Kubernetes machinery runs):**
Pod-A's kernel builds the packet with the source **already filled in**
automatically — this is ordinary IP behavior, not Kubernetes-specific:
```
src: 10.244.1.5 (Pod-A's own pod IP)  →  dst: 10.96.5.20 (Service ClusterIP)
```

**Netfilter — DNAT (nat table, PREROUTING/OUTPUT hook, on Node A):**
kube-proxy's iptables rules for this Service were **pre-programmed** the
moment the EndpointSlice was populated (Part 1, step 12) — kube-proxy is
reactive to endpoint changes, not to individual packets. Multiple backend
pods produce multiple `--probability`-weighted DNAT rules (the
`statistic --mode random` match), giving roughly even distribution. This
packet matches the rule targeting Pod-B:
```
dst rewritten: 10.96.5.20 → 10.244.2.5      (source untouched throughout)
```
**conntrack records** this translation against the connection's 4-tuple —
this is the first of its two engagements.

**FIB lookup (Node A):** routing now operates on the *new* destination.
Longest-prefix match finds the route flanneld installed for Pod-B's subnet:
```
10.244.2.0/24 dev flannel.1
```
This sends the packet to the flannel.1 VXLAN device — the moment it
"enters the overlay."

**FDB lookup (flannel.1, Node A):** the VXLAN device's forwarding database
— populated by flanneld from API-server-derived node/subnet mappings —
resolves the destination subnet to Node B's **real** IP (the VTEP):
```
10.244.2.0/24 → 192.168.1.11
```

**Encapsulation:** flannel.1 wraps the entire original packet as payload
inside a new one:
```
inner:  10.244.1.5 → 10.244.2.5                       (unchanged pod-to-pod)
outer:  192.168.1.10 → 192.168.1.11, UDP dst port 8472, VXLAN header (VNI)
```
A second FIB lookup routes this *outer* packet out Node A's real NIC via
the ordinary connected route (`192.168.1.0/24 dev eth0`).

**Physical network:** routes purely by the outer header — ordinary
node-to-node UDP, oblivious to the pod IPs riding inside.

**Node B — decapsulation:** the kernel recognizes UDP:8472 as VXLAN traffic
(and matches the VNI), hands it to the local `flannel.1` device, which
strips the outer headers and recovers the original inner packet
(`10.244.1.5 → 10.244.2.5`).

**FIB lookup (Node B):** on the recovered inner packet, matches the local
connected route `10.244.2.0/24 dev cni0` → delivered to the `cni0` bridge
→ the veth pair → **Pod-B's `eth0`**.

**Pod-B replies.** Ordinary IP behavior — it swaps source/destination and
sends toward the address it saw as the sender:
```
src: 10.244.2.5  →  dst: 10.244.1.5
```
This return trip is the **mirror image** of the forward path: Node B's FIB
routes it to `flannel.1`, Node B's FDB resolves `10.244.1.0/24` to Node A's
real IP, VXLAN-encapsulates, physical network delivers to Node A,
decapsulates, FIB-routes to `cni0`, reaches Node A's local netfilter path.

**conntrack — the second engagement (on Node A, on the return packet):**
the reply arrives with source `10.244.2.5` — the real pod IP Pod-A never
knowingly talked to. conntrack matches this packet against the mapping it
recorded earlier and **reverses** the translation, rewriting the source
back to the Service IP:
```
10.244.2.5 → 10.244.1.5   becomes   10.96.5.20 → 10.244.1.5
```
Only now does the packet match the 4-tuple Pod-A's socket is actually
waiting on, and it's delivered without being rejected.

---

## The complete layered summary

```
DNS (CoreDNS)         name → Service ClusterIP           (resolution)
kube-proxy (netfilter) Service ClusterIP → Pod IP, DNAT   (translation, + conntrack records)
FIB (routing table)    Pod IP → correct device (flannel.1 or cni0)  (device selection)
FDB (VXLAN device)      subnet → remote node's real IP     (tunnel endpoint selection)
Flannel/VXLAN            pod packet → wrapped in node-to-node UDP  (transport)
conntrack (return path)  real Pod IP → Service IP on the reply      (reversal)
```

Each layer hands off to exactly the next: CoreDNS never sees a pod IP;
kube-proxy never deals with node IPs or encapsulation; Flannel never deals
with Service IPs; conntrack exists solely to make the translation
bidirectional. No component does more than its one job, and the pod itself
is unaware that any of this — DNAT, routing, encapsulation, or its
reversal — ever happened.