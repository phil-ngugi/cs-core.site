# Linux Storage & Kubernetes Volumes

How Linux stores files and how Kubernetes volumes work on top of that.

---

## Part 1 — The storage stack (disk → file)

Think of it as layers. A file write travels top to bottom.

```
Your app: write("hello" to /data/file.txt)   ← just asks the kernel
   │
VFS         ← one unified interface for "files & directories", picks which filesystem
   │
Filesystem  ← ext4 etc. Knows which BLOCKS a file uses. Holds inodes.
   │
Page cache  ← write lands in RAM first (fast!), flushed to disk later
   │
Block layer ← queues the write, talks to the driver
   │
Driver      ← NVMe/SATA commands to the hardware
   │
SSD/HDD     ← actual physical storage
```

**Key point:** the app never touches the disk. It asks the kernel; the kernel handles every layer below.

---

## Part 2 — The core pieces

### Disk vs partition vs filesystem
- **Disk** = the hardware (`/dev/sda`). Never mounted itself.
- **Partition** = a slice of the disk (`/dev/sda1`). Holds one filesystem.
- **Filesystem** = the ext4/xfs/etc. structure written onto a partition (by `mkfs`).
- One disk can have many partitions → many filesystems → many mounts.

### Inode
- Every file/dir = one **inode** = its metadata (size, perms, timestamps) + pointers to which **blocks** hold its data.
- The **filename** is separate — it's a directory entry mapping a name → an inode number.
- Each filesystem has its **own inode table** (created at format time). Inode numbers are unique only *within* one filesystem.
- This is why hard links can't cross filesystems (inode numbers are local).

### How the filesystem tracks free space (NOT the inode table)
- **Block bitmap** — one bit per block: used (1) or free (0). This is how it knows what's empty.
- **Inode bitmap** — which inode slots are used/free (you can run out of inodes even with disk space left → `df -i`).
- **Superblock** — filesystem-wide totals (so `df` is instant).
- Inode table only tracks blocks that *belong to files*, not emptiness.

### Block
- Fixed-size chunk (usually 4KB) the filesystem/device deal in. Files = sets of blocks tracked by the inode.

---

## Part 3 — Mounting

### The big idea
Linux has **ONE tree** rooted at `/` (not C:/ D:/ like Windows). Every disk/filesystem is **grafted onto a path** in that one tree.

```
/           ← sda1 (ext4)      the root
/boot/efi   ← sda15 (FAT32)    a different filesystem, grafted in
/home       ← sdb1  (ext4)     another disk, grafted in
/proc /sys  ← virtual (RAM)    kernel-generated, no disk
```

- **Mounting** = "make this filesystem appear at this path."
- An unmounted disk is invisible — exists as `/dev/sdb` but you can't read its files until mounted.
- What gets mounted is a **filesystem** (usually on a partition), never the bare disk.
- Crossing a mount point (`cd /home`) = VFS silently switches to that filesystem.

### Where mount info lives
- The **mount table** = kernel memory, VFS level, **runtime only**. Rebuilt every boot, gone on reboot.
- The **instructions** persist: `/etc/fstab` (on disk) says what to mount where; kernel reads it at boot and builds the runtime table.
- Live state: `findmnt`, `mount`, `/proc/mounts`.

### Useful commands
```bash
lsblk          # disks, partitions, and where each is mounted
findmnt        # the mount tree (clearest view)
df -h          # each mounted filesystem + usage
df -i          # inode usage
mount /dev/sdb1 /mnt/data    # mount manually
```

---

## Part 4 — Bind mounts

- A normal mount attaches a **filesystem (on a device)** at a path.
- A **bind mount** attaches an **existing directory** at a *second* path: `mount --bind /source /target`.
- No new filesystem, no device — just a new mount record pointing at the **same inode**. Same data, two paths, no copying.
- Same nature as any mount: runtime, in-memory, VFS-level, gone on reboot (unless in fstab with `bind`).

```bash
mount --bind /source /target   # /source now also visible at /target
```

---

## Part 5 — Special / virtual filesystems

- **tmpfs** — backed by RAM, not disk. Fast, ephemeral (gone on reboot).
- **proc, sysfs** — kernel info exposed as files (`/proc`, `/sys`).
- **overlayfs** — stacks read-only layers + a writable layer = container images.
- **loop device** (`/dev/loopN`) — makes a **regular file look like a disk**. No hardware. Used for: ISO images, snap packages (your `/dev/loop*` = snaps), or `losetup` labs.

---

## Part 6 — Kubernetes volumes

### The levels (don't confuse them)
- **PVC** = a *request* for storage ("I need 1Gi").
- **PV** = the *actual* storage resource the PVC binds to.
- PV is *backed by* something real: local-path, NFS, cloud disk, etc.
- So PVC/PV are an **abstraction layer** ABOVE the actual storage.
- Direct volumes (hostPath, configMap, secret, emptyDir) skip PVC — used straight in the pod spec.

### THE universal rule
**Every volume ends as a BIND MOUNT into the container's mount namespace** (that's how anything gets into the container's isolated filesystem). What differs is the **source** of that bind.

```
Volume type        Source of data                      Final step
──────────────────────────────────────────────────────────────────
hostPath           a node path YOU name                bind mount
local / local-path provisioner's isolated node dir     bind mount
PVC (on disk)      a PV-backed disk directory          bind mount
nfs                remote NFS export (network-mounted)  bind mount  (+ nfs mount underneath)
emptyDir           kubelet-made node dir               bind mount
emptyDir (Memory)  tmpfs (RAM)                         bind mount
configMap          API data → written to tmpfs (RAM)   bind mount
secret             API data → written to tmpfs (RAM)   bind mount
```

- **Just a bind mount** (source = node dir): hostPath, local-path, emptyDir, PVC-on-local-disk.
- **Extra mount underneath, then bind**: NFS (network mount first), cloud disk (block-device mount first), configMap/secret/emptyDir-Memory (tmpfs first).

### ConfigMap / Secret specifics
- Data comes from the **API server** (etcd), NOT a disk.
- Kubelet writes each key as a file into a **tmpfs (RAM)** dir, then bind-mounts it in.
- Secrets use tmpfs so they **never hit the node's disk in plaintext** and vanish with the pod.

---

## Part 7 — Volume type comparison (the ones that matter)

| Type | Persists past pod? | Follows pod to other node? | Safe? | Use |
|---|---|---|---|---|
| **emptyDir** | ❌ no (dies with pod) | n/a | ✅ | scratch / share between containers in a pod |
| **hostPath** | ✅ (on that node) | ❌ no | ⚠️ DANGEROUS | node agents only; never for app data |
| **local / local-path** | ✅ (on that node) | ❌ pinned to node | ✅ | node-local persistent data |
| **PVC + PV (NFS/cloud)** | ✅ | ✅ (network/cloud) | ✅ | real persistent app data |
| **configMap / secret** | n/a (from API) | ✅ (re-materialized) | ✅ | config & secrets |

### hostPath danger
- Pod picks the path → can mount `/` → sees/writes ALL node files (secrets, kubeconfigs, SSH keys) → **node/cluster takeover**.
- Blocked by **Pod Security Admission** (baseline/restricted forbid hostPath).

### local-path is safe
- Pod does NOT pick the path — provisioner auto-assigns an **isolated dir** (`/opt/local-path-provisioner/pvc-<uuid>...`). No path control = no hostPath attack. Still node-local (data lost if node dies) — availability limit, not security.

### NFS
- Can only mount what the server **exports** (`/etc/exports`). A pod can't reach a remote `/` unless the server (mis)exports it. `no_root_squash` = extra danger.

---

## One-line mental models

- **A file "is" its inode**; the name just points at it.
- **Free space** = block bitmap, not the inode table.
- **One tree**, every disk grafted in via a mount.
- **Mount state** = runtime/RAM; **fstab** = the persistent instructions.
- **Bind mount** = same inode, two paths, no copy.
- **Every k8s volume** = a bind mount into the container; only the *source* differs.
- **hostPath picks the path (dangerous); local-path's provisioner picks it (safe).**
- **configMap/secret** = API data → tmpfs → bind-mounted (RAM, never plaintext on disk).