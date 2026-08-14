# Kubernetes Deep-Dive Checklist

Tracking what's been covered at full mechanism depth vs. what's remaining.
Two tiers of concept: **Linux/kernel-backed** (Kubernetes orchestrating real
OS primitives) and **application-level** (pure API-server/etcd/Go-controller
logic, no kernel underneath). See note at the bottom.

Legend: ✅ = covered in depth (mechanism-level) · ⬜ = not yet covered

---

## Container Runtime & Isolation (Linux-backed)

- ✅ Namespaces generically (what a namespace is, the 8 types)
- ✅ `task_struct` / `nsproxy` — where namespace pointers actually live in the kernel
- ✅ PID namespace mechanics (`struct pid`, one process/multiple PIDs)
- ✅ cgroups (CPU/memory limiting) — conceptual, not yet cgroup-v2-internals depth
- ✅ `clone()` / `unshare()` / `setns()` — the three syscalls that do everything
- ✅ Pause container — what it is, why it exists, which namespaces it holds (net/IPC/UTS)
- ✅ Why PID and mount are NOT shared pod-wide by default
- ✅ Init containers — sequencing mechanism (kubelet waits for exit 0)
- ✅ Sidecar containers — legacy vs native (`restartPolicy: Always` on initContainers)
- ✅ Ephemeral containers — debug use, PID namespace joining
- ✅ OCI spec vs CRI spec — what each standardizes
- ✅ runc (OCI runtime) — what it does and does NOT do (no networking)
- ✅ containerd (CRI runtime) — orchestrates both runc AND CNI
- ✅ Docker vs containerd vs CRI-O vs Podman — full comparison, CNM vs CNI
- ✅ Why Windows/Linux containers can't cross — shared kernel requirement
- ✅ Why Arch-on-RHEL-host works — same kernel, different userland only
- ✅ How container images are built (layers, Dockerfile, OCI image spec)
- ✅ `RunPodSandbox` / `CreateContainer` / `StartContainer` — the actual CRI calls, in sequence

## Networking (Linux-backed)

- ✅ CNI spec — what it standardizes, who calls it (containerd, NOT kubelet, NOT runc)
- ✅ CNI binary resolution (`type` → `/opt/cni/bin`, containerd's `bin_dir`)
- ✅ Reference CNI plugins (bridge, host-local, loopback) — what each does
- ✅ Pod IP allocation flow — CNI → containerd → kubelet → API server (report, not fetch)
- ✅ Flannel architecture — daemon (control-plane work) vs CNI binary (per-pod)
- ✅ Flannel routes subnets PER NODE, not per pod
- ✅ VXLAN encapsulation — full mechanism, inner vs outer headers
- ✅ FIB (routing table / L3) vs FDB (VXLAN forwarding database / L2) — the distinction
- ✅ How routes are created/managed — kernel automatic (connected) vs explicit (flanneld)
- ✅ Mount namespace analog applied to networking — same setns() pattern
- ✅ Calico vs Flannel vs reference plugins — design tradeoffs (BGP vs overlay)
- ✅ kube-proxy — iptables DNAT mechanism, `--probability` load balancing
- ✅ Service ClusterIP — why it's virtual (no interface, exists only as iptables rules)
- ✅ conntrack — what it is, when it engages (record on DNAT, reverse on reply), why needed
- ✅ CoreDNS — architecture, authoritative vs forwarding, why NOT direct pod-IP resolution
- ✅ Headless Services — the exception where CoreDNS returns pod IPs
- ✅ DNS troubleshooting causes (resolv.conf, endpoints vs pod status, NetworkPolicy blocking :53)
- ✅ Full pod-to-pod-via-Service packet trace (DNS → DNAT → FIB → FDB → VXLAN → reverse)
- ✅ Ports inside pods — per-pod bind table, containerPort vs Service port vs targetPort
- ✅ NetworkUnavailable node condition — why no-CNI causes NotReady
- ✅ Node hostNetwork — mechanism, security implications, RBAC/PSA gating

## Storage (Linux-backed)

- ✅ Full disk→file write path (VFS → filesystem → page cache → block layer → driver → device)
- ✅ Inodes — what they are, where stored, per-filesystem tables
- ✅ Block bitmap / inode bitmap / superblock — how free space is actually tracked
- ✅ Mounting — the one-tree model, mount records as runtime VFS state (not persistent)
- ✅ `/etc/fstab` vs the live mount table — persistent instructions vs runtime state
- ✅ Bind mounts — exact mechanism (same inode, new mount record, no copy)
- ✅ Loop devices — file-as-block-device
- ✅ Disk vs partition vs filesystem — precise distinction
- ✅ Every Kubernetes volume type traced to its bind-mount mechanism
- ✅ ConfigMap/Secret — tmpfs materialization → bind mount (why tmpfs, security reason)
- ✅ hostPath vs local vs local-path — mechanism AND security comparison (path control)
- ✅ hostPath `/` — full attack mechanism and why it's blocked (PSA)
- ✅ PV / PVC / StorageClass object model and relationship
- ✅ StorageClass generic anatomy (provisioner, parameters, reclaimPolicy, volumeBindingMode)
- ✅ Static vs dynamic provisioning — full flow for both
- ✅ CSI architecture — controller (Deployment) vs node plugin (DaemonSet), why each
- ✅ CSI driver naming convention, exact-string matching, multiple classes/one provisioner
- ✅ `csi-provisioner` sidecar — what it watches, `CreateVolume`, `claimRef` pre-binding
- ✅ `PersistentVolumeController` — binds via direct claimRef match (dynamic) or search (static)
- ✅ CSI node plugin registration — Unix socket + hostPath + `plugins_registry/`, kubelet's local map
- ✅ `CSINode` object — API-visible mirror of local registration
- ✅ Hands-on: static local PV, NFS server setup, NFS CSI driver, dynamic PVC binding
- ⬜ cgroup-level I/O throttling / storage QoS

## Pod & Cluster Lifecycle States

- ✅ Pod phases (Pending, Running, Succeeded, Failed, Unknown) — exact meaning + mechanism
- ✅ Container states (Waiting/Running/Terminated + reasons)
- ✅ Pod conditions (PodScheduled, Initialized, ContainersReady, Ready) — diagnostic order
- ✅ Node conditions (Ready, NetworkUnavailable, MemoryPressure, DiskPressure, PIDPressure)
- ✅ `Unknown` phase — node heartbeat loss mechanism, eviction/taint follow-up
- ✅ Termination sequence (SIGTERM → grace period → SIGKILL), stuck-Terminating causes
- ✅ Events — Normal vs Warning, expiry, diagnostic ordering
- ✅ Deployment/StatefulSet/DaemonSet status fields and what differs between them
- ✅ Full diagnostic command sequence for "something's wrong"

## Kubernetes Object Model / API Machinery

- ✅ Deployment → ReplicaSet → Pod chain (why you don't write ReplicaSets directly)
- ✅ Labels vs annotations — selection vs pure metadata, real-world usage patterns
- ✅ Downward API — mechanism (env vars vs files), why files update live, why used over API queries
- ✅ Imperative command generation (`$dr`/`--dry-run`) — full command reference

---

## NOT yet covered (remaining)

### Still Linux/kernel-adjacent
- ⬜ Scheduling — filter/score mechanism (app-level decision, kernel-level consequence)
- ⬜ Resource requests/limits at the cgroup enforcement level (CPU throttling, OOMKill specifics)
- ⬜ QoS classes (Guaranteed/Burstable/BestEffort) and eviction ordering

### Pure application-level (API server / etcd / Go-controller logic — no kernel underneath)
- ⬜ RBAC — authn (certs/tokens/OIDC) → authz (Role/ClusterRole matching) full request pipeline
- ⬜ Admission control — mutating/validating webhooks, Pod Security Admission mechanism
- ⬜ etcd internals — Raft consensus detail, watch mechanism, backup/restore mechanics
- ⬜ Certificates/PKI — cluster CA, what signs what, TLS bootstrap, rotation
- ⬜ CRDs — how the API server dynamically handles a new type
- ⬜ Operators — reconciliation loop in practice (e.g. CloudNativePG)
- ⬜ HPA / VPA / Cluster Autoscaler — metrics-server, replica math
- ⬜ Helm — templating, release tracking (stored as a Secret)
- ⬜ Kustomize — overlays/patches
- ⬜ Gateway API + HTTPRoute — not yet implemented (only CRDs/MetalLB/Istio foundation laid)
- ⬜ Istio service mesh — mTLS, traffic splitting, AuthorizationPolicy
- ⬜ PodDisruptionBudget
- ⬜ Node drain/cordon — exact mechanism (cordon + evict, PDB respect)
- ⬜ Static pods — kubelet-manifest-directory mechanism, independent of API server
- ⬜ kubeadm upgrade — not yet performed
- ⬜ etcd backup/restore — not yet performed hands-on
