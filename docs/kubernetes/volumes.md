# Kubernetes Storage Deep Dive — PV, PVC, StorageClass & CSI

A complete reference tracing exactly how a PVC turns into mounted storage in a container,
using our real NFS setup as the working example.

---

## Part 1 — The object model

```
StorageClass   = a RECIPE ("who provisions storage, and how")
PersistentVolume (PV)   = the ACTUAL storage resource (a real directory/disk somewhere)
PersistentVolumeClaim (PVC) = a REQUEST for storage ("I need 1Gi, ReadWriteOnce")
CSI driver     = the software that DOES the provisioning + mounting
```

- A **PVC never names a provisioner directly** — it only names a `storageClassName`.
- The **StorageClass** is what carries `provisioner: <name>`.
- If a PVC omits `storageClassName`, the `DefaultStorageClass` admission controller
  auto-fills it with whichever class is annotated
  `storageclass.kubernetes.io/is-default-class: "true"`. Only one class should carry
  this annotation — two defaults makes class-less PVCs ambiguous.

---

## Part 2 — What a StorageClass generically needs

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io      # WHO does the work (must match a registered CSI driver, exact string)
parameters:                       # HOW/WHERE — backend-specific, passed straight to the provisioner
  server: 192.168.252.2
  share: /srv/nfs/k8s
reclaimPolicy: Retain             # what happens to storage when the PVC is deleted (Delete | Retain)
volumeBindingMode: Immediate      # WHEN to provision (Immediate | WaitForFirstConsumer)
mountOptions:
  - nfsvers=4.1
```

| Field | Why it exists |
|---|---|
| `provisioner` | Names the exact CSI driver string that must be registered in-cluster (`kubectl get csidrivers`). Not arbitrary — hardcoded by the driver's authors. |
| `parameters` | Free-form; the provisioner decides what it needs (NFS: server+share; cloud disk: type/zone/iops). |
| `reclaimPolicy` | `Delete` = destroy storage when PVC is deleted. `Retain` = keep it (safer for labs/important data). |
| `volumeBindingMode` | `Immediate` = provision right away (fine for network storage). `WaitForFirstConsumer` = wait until a pod is scheduled, because the provisioner needs to know WHICH NODE (required for node-local storage like local-path). |

**Multiple StorageClasses CAN share one provisioner** — e.g. `nfs-fast` and `nfs-archive`
both using `nfs.csi.k8s.io` but pointing at different exports/reclaim policies. The
provisioner just reads whatever `parameters` came with the PVC's specific class.

**Naming convention** for provisioner strings: `name.csi.domain` (reverse-DNS style,
community convention, not enforced) — e.g. `nfs.csi.k8s.io`, `ebs.csi.aws.com`,
`pd.csi.storage.gke.io`, `rancher.io/local-path` (older style).

---

## Part 3 — What a CSI driver actually installs

Installing a CSI driver (e.g. `csi-driver-nfs`) creates:

```
CSIDriver object          → registers "nfs.csi.k8s.io" formally with the cluster
Deployment: csi-nfs-controller   → the PROVISIONING brain (NOT a DaemonSet — just 1-2 pods,
                                     anywhere the scheduler puts them, since NFS is network
                                     storage and doesn't need to be on a specific node)
    containers (4):
      - csi-provisioner   → generic sidecar: watches PVCs, calls CreateVolume
      - csi-resizer       → generic sidecar: handles PVC resize requests
      - csi-snapshotter   → generic sidecar: handles VolumeSnapshots
      - nfs               → the ACTUAL NFS-specific driver logic

DaemonSet: csi-nfs-node    → the MOUNTING agent — one pod per node (must be
                              per-node, because mounting happens on whichever
                              node the consuming pod lands on)
    containers (3):
      - node-driver-registrar → tells the kubelet where this driver's socket is
      - liveness-probe        → health check
      - nfs                   → same driver binary, handling the real per-node mount

RBAC: ServiceAccounts, ClusterRoles, ClusterRoleBindings
```

Does NOT include a StorageClass — you write that yourself with your own server/share.

Install methods (raw `kubectl apply -k <repo-url>` often does NOT work — no top-level
kustomization.yaml at that path for every version/driver). Use the documented method:
```bash
# Option A — install script
curl -skSL https://raw.githubusercontent.com/kubernetes-csi/csi-driver-nfs/v4.9.0/deploy/install-driver.sh | bash -s v4.9.0 --

# Option B — Helm (more standard/production-typical)
helm repo add csi-driver-nfs https://kubernetes-csi.github.io/csi-driver-nfs
helm install csi-driver-nfs csi-driver-nfs/csi-driver-nfs -n kube-system --version 4.9.0
```

---

## Part 4 — The full dynamic provisioning sequence

```
1. PVC created, referencing storageClassName: nfs-csi
        │
2. csi-provisioner (watches PVCs, filters by matching provisioner NAME string) sees it,
   reads the StorageClass's parameters (server/share)
        │
3. Creates the actual storage:
   - inside the controller pod's "nfs" container, in ITS OWN mount namespace:
       mount -t nfs <server>:<share> /internal/path
       mkdir /internal/path/pvc-<uuid>
       umount /internal/path
   (brief, private, unrelated to any pod's eventual mount)
        │
4. csi-provisioner CREATES the PV object itself, setting:
   - spec.csi.driver = nfs.csi.k8s.io          ← tells kubelet later WHICH driver to call
   - spec.csi.volumeAttributes = {server, share, subdir}
   - spec.claimRef = <exact PVC: name/namespace/uid>   ← pre-pins the binding target
        │
5. PersistentVolumeController (inside kube-controller-manager, NOT a separate
   "PV controller" — one controller watching BOTH PVCs and PVs):
   sees claimRef already set → binds DIRECTLY (no search needed)
   → patches PV.status.phase=Bound, PVC.spec.volumeName=<pv>, PVC.status.phase=Bound
        │
6. Pod referencing the PVC is created
   - Immediate mode: PVC already Bound → scheduling proceeds completely normally
   - WaitForFirstConsumer mode: scheduler places the pod FIRST (normal criteria) →
     THAT triggers steps 2-5 to happen now, node-aware (needed for local-path,
     where the provisioner must know which node before creating the directory)
        │
7. Pod scheduled to a node → kubelet processes its volumes:
   - reads the BOUND PV → gets spec.csi.driver = nfs.csi.k8s.io
   - looks up that driver name in its LOCAL (this node's) registered-drivers map
   - calls that driver's NODE PLUGIN (the DaemonSet pod ON THIS NODE) via
     NodePublishVolume, passing volumeAttributes (server/share/subdir)
        │
8. Node plugin does the REAL, lasting  mount -t nfs <server>:<share>/pvc-<uuid>  on THIS node
        │
9. kubelet BIND-MOUNTS that mounted path into the container's mountPath
```

### Static provisioning — the difference

You hand-write the PV yourself, with NO `claimRef`, phase starts `Available`.
The `PersistentVolumeController` then SEARCHES unbound PVCs against Available PVs,
matching `storageClassName` + `accessModes` + `capacity` (+ `nodeAffinity` if
present), and binds the first suitable match — same controller, different path
(search instead of direct-bind-via-pre-set-claimRef).

---

## Part 5 — How the kubelet knows WHICH driver socket to call (local, not API-server)

This is entirely **node-local**, filesystem + Unix-socket based — deliberately
independent of the API server (kubelet must manage local volumes even if the API
server is briefly unreachable).

```
Node plugin pod (on THIS node):
   "nfs" container exposes gRPC over a Unix socket INSIDE itself: /csi/csi.sock
        │
   hostPath volume mounts that socket out onto the host's real filesystem:
        /var/lib/kubelet/plugins/nfs.csi.k8s.io/csi.sock
        │
   "node-driver-registrar" sidecar:
        asks the driver its name (GetPluginInfo) → writes a registration
        entry into:
        /var/lib/kubelet/plugins_registry/
        │
   KUBELET (a host process) WATCHES that directory directly (filesystem watch,
   no API server involved) → connects, confirms driver name → stores in its
   OWN IN-MEMORY MAP:  "nfs.csi.k8s.io" → /var/lib/kubelet/plugins/nfs.csi.k8s.io/csi.sock
```

Two SEPARATE lookups converge at mount time:
- **API server** → tells kubelet WHICH driver name a PV needs (`spec.csi.driver`)
- **Local file/socket registration** → tells kubelet WHERE that driver's socket
  actually lives ON THIS NODE

### How to inspect it yourself

```bash
# on any node:
ls -la /var/lib/kubelet/plugins/            # registered drivers + their sockets
ls -la /var/lib/kubelet/plugins_registry/   # registration entries

# from kubectl — the API-visible MIRROR of local registration (for scheduler/
# topology awareness; NOT the registration mechanism itself):
k get csinodes
k get csinode <node-name> -o yaml
```

---

## Part 6 — Node-local vs network storage — why some things differ

| | local-path | NFS |
|---|---|---|
| Controller can run on any node? | doesn't matter, but directory MUST be on the pod's node | yes — network reachable from anywhere |
| `volumeBindingMode` | `WaitForFirstConsumer` (must know the node first) | `Immediate` is fine |
| Directory created on | ONE node only — whichever the first pod using the PVC lands on | one export, reachable from ALL nodes |
| Access mode | `ReadWriteOnce` only (node-pinned) | `ReadWriteMany` possible (network-shared) |
| PV carries | `nodeAffinity` pinning it to that one node | just `server`/`share`/`subdir` |
| Node dies | data lost/unreachable | data safe (lives on the NFS server) |

**local-path directory is created on exactly ONE node** — the one the first
consuming pod is scheduled to (never proactively on all nodes). Rescheduling that
pod later is forced back to the same node via the PV's `nodeAffinity`.

---

## Part 7 — Quick command reference

```bash
# StorageClasses
k get storageclass                                  # (default) marker shown here
k describe storageclass nfs-csi

# CSI driver health
k get csidrivers
k get pods -n kube-system -l app=csi-nfs-controller
k get pods -n kube-system -l app=csi-nfs-node -o wide
k get csinodes                                       # per-node registered drivers

# PV / PVC
k get pv
k get pvc
k describe pvc <name>                                 # see Events if stuck Pending
k describe pv <name>                                   # see claimRef, csi driver/attributes

# NFS server side (on the export host)
sudo exportfs -v
cat /etc/exports
```

---

## One-line mental models

- **PVC never names a provisioner** — only a StorageClass; the class names the provisioner.
- **StorageClass = recipe, not storage.** Nothing is "backed" by a third deployable thing —
  you need the provisioner (CSI driver, deployed once) + the real storage (export/disk, set up once).
- **The provisioner watches PVCs, not PVs** — the PV is an OUTPUT of provisioning, not a trigger.
- **claimRef is set BY the provisioner AT creation** — binding isn't guessed/matched after the fact
  for dynamic provisioning; it's pre-wired.
- **Binding = the `PersistentVolumeController` patching both objects via the API server** —
  one controller, watches both PVC and PV, same "owns a relationship between two resource
  types" pattern as EndpointSlices or Deployments/ReplicaSets.
- **Controller creates storage; NODE PLUGIN does the real mount** — two different components,
  two different triggers (PVC-watch vs kubelet-call-at-pod-startup), and the real mount only
  ever happens on the specific node the consuming pod lands on.
- **Driver registration is 100% local** (Unix socket + hostPath + a directory the kubelet
  watches) — the API server is never involved in registration, only in telling the kubelet
  *which* driver name a given PV needs.