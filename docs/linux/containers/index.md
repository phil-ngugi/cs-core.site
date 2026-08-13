# The Concept of Containerization / Isolation

## A container is not a kernel object

In Linux, there is **no struct for creating "containers."** Instead there is a process struct called **`task_struct`** that holds information about a process. Even inside a container, the kernel sees the process as **just a normal Linux process** — with a normal host PID, scheduled like any other.

Isolation comes from **pointers** inside the `task_struct`. One of these is `nsproxy`, which points at the process's namespaces:

```c
struct task_struct {
    ...
    struct nsproxy *nsproxy;   // ← pointer to this process's namespaces
    ...
};
```

And `nsproxy` is a struct of pointers, one per namespace type:

```c
struct nsproxy {
    struct uts_namespace    *uts_ns;             // hostname
    struct ipc_namespace    *ipc_ns;             // IPC
    struct mnt_namespace    *mnt_ns;             // filesystem mounts
    struct pid_namespace    *pid_ns_for_children;// PID
    struct net              *net_ns;             // network stack
    struct time_namespace   *time_ns;            // clock
    struct cgroup_namespace *cgroup_ns;          // cgroup view
    ...
};
```

**The core idea:** if two processes share the same pointer (e.g. the same `net_ns`), they share that namespace — they see the same network stack, same IP, same ports. Different pointer = isolated.

That sharing, paired with **cgroups** (which impose CPU/memory limits), is what we *call* a container.

> **A container is not a kernel object — it's a process whose `task_struct->nsproxy` points at non-default namespace structs, constrained by cgroups.**

OCI runtimes (like runc) just orchestrate the kernel primitives — `clone()`, `unshare()`, `setns()` — to set those pointers and limits. Docker/Kubernetes sit above that.

## Host processes vs container processes

- **Normal host processes** point at the **initial/default** namespaces (`init_net`, etc.) — they all share the host's network, PID tree, filesystem. No isolation between them.
- **A "container" process** points at **freshly-created** namespace structs — so it sees its own network, PID space, filesystem.

Same kind of process. The only difference is *which* namespace structs the pointers reference.

## How one process can have two PIDs

A single `task_struct` (one real process) is stored with its PID **in every namespace it's visible in**:

- host PID namespace → e.g. `48213`
- its own PID namespace → `1`

Same process, two numbers, stored simultaneously (in `struct pid`). The kernel returns whichever number matches the asker's namespace. This is why the host sees a normal PID while the process "inside" sees itself as PID 1. PID namespaces are one-directional: the host can see the process; the process can't see out.

---

# Pod Container Types

## The pause container — the anchor

The pause container is a tiny process that does nothing but **sleep forever**. Its only job is to **hold open the Network, IPC, and UTS namespaces** so other containers can join them.

- runc creates it with fresh net/IPC/UTS namespaces.
- The CNI wires up its network namespace (gives it the pod IP).
- Every app container then **joins** (via `setns()`) those same three namespaces — that's what makes them "one pod": same IP, same ports, same hostname.

It never restarts (it can't crash), so it's a stable anchor — app containers can restart without the pod losing its network identity.

**What's shared pod-wide (via pause):** Network, IPC, UTS.
**What's NOT shared (each container keeps its own):** PID and Mount — unless `shareProcessNamespace: true` is set for PID. This is why each container can run a **different image** (own filesystem) yet share one IP.

## Init containers

Processes the kubelet runs **one at a time, in order, before the main containers start**. Each must **exit successfully (0)** before the next runs. If any fails, the app containers don't start.

Kernel-wise they're identical to any container — the "init" behavior is pure sequencing (run → wait for exit → next), not a special kernel feature.

Common uses: fix volume permissions, wait for a dependency, run DB migrations.

## Sidecar containers

Helper containers that run **alongside** the main container for the pod's life — logging agents, proxies (e.g. Istio's Envoy), etc. They **share the pod's namespaces** (network/IPC/UTS), so a logging sidecar can, for example, share a volume with the app and read its logs.

Two forms:
- **Legacy sidecar** — just a normal entry in `containers:`. No special keyword; "sidecar" is only a convention.
- **Native sidecar** — an entry under `initContainers:` **with `restartPolicy: Always`**. That field is what makes it a sidecar: it starts *before* the app containers (init-style) but *stays running* alongside them (and shuts down after). Fixes ordering problems the legacy form had.

## Ephemeral containers

Temporary containers injected into an **already-running** pod for debugging (`kubectl debug`). Often join a target container's PID namespace so you can inspect its processes — useful when the app image has no shell/tools. Can't be removed once added.

---

## One-line reminders

- Container = a **process** with non-default namespace pointers + cgroup limits. No kernel "container" object.
- Host sees a **normal process** with a normal PID; the process sees itself as **PID 1** (two PIDs, one process).
- **pause** holds **net/IPC/UTS**; app containers `setns()` into them → "a pod."
- **PID and mount are NOT shared** by default → each container keeps its own process view and filesystem.
- **init** = run in order, each to exit, before the app starts.
- **sidecar** = runs alongside; native sidecar = `initContainers` entry with `restartPolicy: Always`.
- **ephemeral** = debug container added to a live pod.