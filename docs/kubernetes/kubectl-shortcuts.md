# CKA Command Reference (v1.35 curriculum)

Domain weights: Troubleshooting 30% · Cluster Architecture 25% · Services & Networking 20% · Workloads & Scheduling 15% · Storage 10%. Pass = 66%, open-book on official K8s docs only.

---

## Setup — type these first, every session

```bash
alias k=kubectl
export do="-o yaml"
export dr="--dry-run=client -o yaml"
export now="--force --grace-period=0"
source <(kubectl completion bash)
complete -o default -F __start_kubectl k
# per-task, so you stop typing -n:
k config set-context --current --namespace=<ns>
```

---

## Pods & Deployments — imperative speed core (Workloads 15%)

```bash
k create ns ops
k run web --image=nginx:1.27 --port=80              # single pod (--port, NOT "-- port")
k run tmp --image=busybox --rm -it --restart=Never -- sh   # throwaway debug pod
k run probe --image=busybox --command -- sleep 3600 # override container command
k create deploy api --image=httpd --replicas=3
k scale deploy api --replicas=5
k set image deploy/api httpd=httpd:2.4              # rolling update
k rollout status deploy/api
k rollout undo deploy/api                           # roll back
k rollout history deploy/api
k edit deploy api
k delete pod web $now                               # force-delete stuck pod
```

**Generate → edit → apply** (for anything imperative can't fully express):

```bash
k run probe --image=busybox $dr --command -- sleep 3600 > probe.yaml
k create deploy api --image=httpd $dr > api.yaml
k apply -f api.yaml
```

---

## Services & Exposing (Networking 20%)

**Rule:** `expose` when an object already exists (copies its labels into the selector). `create svc` only when there's nothing to point at.

```bash
k expose pod web --name=web-svc --port=80
k expose deploy api --name=api-svc --port=80 --target-port=8080
k expose deploy api --type=NodePort --port=80
k create svc clusterip standalone --tcp=80:8080     # service with NO existing selector
```

---

## RBAC — heavily weighted, memorize cold (Architecture 25%)

```bash
k create sa deployer
k create role pod-reader --verb=get,list,watch --resource=pods
k create rolebinding deployer-rb --role=pod-reader --serviceaccount=ops:deployer
k create clusterrole node-reader --verb=get,list,watch --resource=nodes
k create clusterrolebinding deployer-crb --clusterrole=node-reader --serviceaccount=ops:deployer
k create rolebinding alice-rb --clusterrole=view --user=alice   # ClusterRole via RoleBinding = scoped to that ns
```

**Verify — often the graded step (must return `yes`):**

```bash
k auth can-i list pods --as=system:serviceaccount:ops:deployer -n ops
k auth can-i '*' '*' --as=alice
k auth can-i --list --as=system:serviceaccount:ops:deployer -n ops
```

---

## ConfigMaps & Secrets (Workloads 15%)

```bash
k create configmap app-cfg --from-literal=KEY=val --from-literal=K2=v2
k create configmap app-cfg --from-file=./config.txt
k create configmap app-cfg --from-env-file=./app.env
k create secret generic db-sec --from-literal=password=s3cr3t
k create secret docker-registry regcred --docker-server=R --docker-username=U --docker-password=P
k create secret tls my-tls --cert=tls.crt --key=tls.key
```

Consuming via `envFrom` / `valueFrom` / `volumes` is **YAML only** — know the shapes.

---

## Scheduling (Workloads & Scheduling 15%)

```bash
k label node node-1 disktype=ssd
k taint node node-1 key=value:NoSchedule
k taint node node-1 key=value:NoSchedule-           # trailing dash removes the taint
k cordon node-1
k uncordon node-1
k drain node-1 --ignore-daemonsets --delete-emptydir-data
```

`nodeSelector`, affinity, tolerations, topologySpreadConstraints → **YAML only**. `nodeName:` in a pod spec bypasses the scheduler (a troubleshooting fact).

---

## Storage (10%)

PV / PVC / StorageClass have **no imperative create** — reproduce YAML from docs fast.

```bash
k get pv,pvc,sc
k get pvc -o wide     # STATUS Bound? which PV?
```

Pending PVC → check `storageClassName` match, `accessModes`, capacity.

---

## Troubleshooting — highest weight, pure speed + method (30%)

**Cluster level:**

```bash
k get pods -A -o wide
k get pods --field-selector status.phase!=Running -A
k describe pod <p>                 # read the Events at the bottom FIRST
k logs <p>
k logs <p> --previous              # crashed container's prior run
k logs <p> -c <container>
k logs deploy/api
k exec -it <p> -- sh
k get events --sort-by=.lastTimestamp -A
k top nodes ; k top pods           # needs metrics-server
k get --raw /healthz               # apiserver health
```

**Node / control-plane level (SSH to the node):**

```bash
systemctl status kubelet
journalctl -u kubelet -f
crictl ps ; crictl ps -a
crictl logs <container-id>
ls /etc/kubernetes/manifests/      # static pod manifests
cat /var/lib/kubelet/config.yaml
```

---

## Cluster Lifecycle — etcd & kubeadm, memorize verbatim (Architecture 25%)

**etcd backup** (near-certain task):

```bash
ETCDCTL_API=3 etcdctl snapshot save /opt/snap.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

**etcd restore:**

```bash
ETCDCTL_API=3 etcdctl snapshot restore /opt/snap.db \
  --data-dir=/var/lib/etcd-restore
# then repoint the etcd static pod's data-dir/volume at the new dir and restart
```

**kubeadm upgrade (per node):**

```bash
kubeadm upgrade plan
kubeadm upgrade apply v1.35.1        # first control-plane node
kubeadm upgrade node                 # other nodes
k drain <node> --ignore-daemonsets
apt-get install -y kubelet=1.35.1-* kubectl=1.35.1-*
systemctl daemon-reload && systemctl restart kubelet
k uncordon <node>
```

**Certs:**

```bash
kubeadm certs check-expiration
kubeadm certs renew all
```

---

## CSR flow — new user access (Architecture 25%)

```bash
openssl genrsa -out alice.key 2048
openssl req -new -key alice.key -out alice.csr -subj "/CN=alice"   # CN=user, O=group
# create a CertificateSigningRequest object with base64 of alice.csr, then:
k get csr
k certificate approve alice-csr
k get csr alice-csr -o jsonpath='{.status.certificate}' | base64 -d > alice.crt
k config set-credentials alice --client-key=alice.key --client-certificate=alice.crt --embed-certs
k config set-context alice --cluster=<c> --user=alice
```

---

## Networking — NetworkPolicy, Ingress, Gateway API (20%)

All **YAML** — pull skeletons from docs. Classic task: default-deny-all ingress, then allow from a specific `podSelector`.

```bash
k get netpol -A
k get ingress,gateway,httproute -A
```

---

## Packaging — Helm & Kustomize (Architecture 25%)

```bash
helm repo add <name> <url> ; helm repo update
helm install myrel <repo>/<chart> --values vals.yaml
helm upgrade myrel <repo>/<chart> --set key=val
helm list ; helm uninstall myrel
helm template <chart>              # render without installing

kubectl kustomize ./overlay       # render
kubectl apply -k ./overlay        # apply
```

---

## Universal speed habits

```bash
k get <res> <name> $do            # dump any object's YAML
k get <res> -o wide
k get <res> -o jsonpath='{.spec.field}'
k explain pod.spec.containers     # field reference, offline
k <cmd> -h                        # fastest flag recall — USE THIS, don't leave blank
```

---

## Priorities

**Burn into muscle memory** (commonly missed):
- `expose` vs `create svc clusterip`
- `run --port` and `run --command -- <cmd>`
- the three-command RBAC combo (`create role` / `rolebinding` with `--verb` / `--resource` / `--serviceaccount`)
- always append `-n <ns>` even when context is set

**Memorize verbatim** (no room to improvise):
- etcd snapshot save/restore with cert paths
- kubeadm upgrade sequence
- CSR flow

**Know the shape, pull skeleton from docs:**
- PV / PVC / StorageClass
- NetworkPolicy
- affinity / tolerations
- Ingress / Gateway / HTTPRoute
- multi-container & volume pod specs

**Meta-skill:** when you forget a flag, `k <cmd> -h` in 2 seconds beats bailing. Blank = zero; `-h` = full marks a few seconds slower.