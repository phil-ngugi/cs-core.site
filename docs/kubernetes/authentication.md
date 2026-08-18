# PKI, TLS, and How Kubernetes Actually Does Identity

A walk from the mathematics of public-key cryptography up to why `admin.conf` is a
certificate but your pod carries a JWT — and why neither could do the other's job.

---

## Part 1 — The PKI Foundations

### 1.1 Keys are asymmetric, and only one half is secret

A keypair rests on a **trapdoor one-way function**: easy to compute forward,
infeasible to reverse, unless you hold a secret.

**RSA.** Pick primes `p, q`. Let `n = pq` and `φ(n) = (p−1)(q−1)`. Choose `e`
(conventionally 65537) and compute `d` such that:

```
e · d ≡ 1  (mod φ(n))
```

Public key is `(n, e)`; private key is `d`. The congruence guarantees that for any `m`:

```
(m^d)^e ≡ m  (mod n)
```

- **Signing:** `s = h^d mod n`, where `h` is the padded hash
- **Verifying:** check `s^e mod n == h`

Anyone holding `(n, e)` can verify. Recovering `d` requires `φ(n)`, which requires
factoring `n` — the hard problem.

**ECDSA.** A curve with base point `G` of order `n`. Private key `d` is a scalar;
public key `Q = dG`. Computing `Q` from `d` is fast; recovering `d` from `Q` is the
**discrete logarithm problem**.

Sign with hash `z` and nonce `k`:

```
R = kG,  r = x-coordinate of R
s = k⁻¹(z + r·d)  mod n
```

Verify with only `Q`:

```
u₁ = z·s⁻¹,  u₂ = r·s⁻¹
check x-coordinate of (u₁G + u₂Q) == r
```

Substituting `Q = dG` collapses that sum back to `kG` — it matches only if the signer
knew `d`.

**The point of both:** verification is a *public* computation. No secret, no network
call, no contact with the signer. That is what makes offline, independent verification
possible, and it is the property everything else is built on.

### 1.2 Hashing and signing are separate steps

A common muddle. The private key does not create the hash.

1. **Hash** — run the to-be-signed content through SHA-256. No keys involved.
   Deterministic and public; anyone can compute it.
2. **Sign** — apply the private key to that digest.

```
signature = Sign(privkey, SHA256(content))
```

Hashing alone gives integrity but not authenticity — a tamperer could simply recompute
it. Signing binds the digest to an identity. Hashing first is also practical: RSA and
ECDSA operate on small fixed-size inputs, so you sign 32 bytes rather than a whole
document.

### 1.3 What a certificate is

`server.crt` contains **no private keys whatsoever**. It is a public document handed to
strangers.

| Contents | |
|---|---|
| Subject public key | the key being vouched for |
| Identity | subject/CN, SANs, issuer, validity, serial, extensions |
| Signature | made by the CA over all of the above |

Where the four keys actually live:

| Key | Location |
|---|---|
| Server **private** | `server.key`, never leaves the host |
| Server **public** | inside the certificate |
| CA **private** | offline / HSM at the CA; used *to make* the signature, not stored in it |
| CA **public** | inside the CA's own certificate, in your trust store |

Verification: read the cert, see the issuer name, find that CA's cert in the trust
store, use the **CA public key** to check the signature. Valid signature means the
binding of *name → public key* is authentic.

### 1.4 Two signatures, two purposes

Both exist, made at different times by different parties, and they end up in different
places.

| | CSR signature | Certificate signature |
|---|---|---|
| Signed by | the **subject's** private key | the **CA's** private key |
| Covers | public key + requested subject/SANs | the tbsCertificate |
| Purpose | proof-of-possession | attestation of identity |
| Verified by | the CA, using the key inside the CSR | every client, using the CA's public key |
| Lifetime | discarded after issuance | lives in the cert, checked forever |

The CA does **not** layer its signature on top of yours. It validates your CSR, strips
the fields it wants, adds its own (serial, validity, issuer), and signs that as a
freshly constructed document.

### 1.5 Why a public key being public is harmless

Given a server's public key, a middleman still cannot obtain a certificate for it:

1. **The CSR must be self-signed.** Proof-of-possession fails without the private key.
2. **Domain control validation.** A public CA demands an HTTP token, DNS record, or
   admin email before issuing.
3. **The certificate would be useless anyway.** Completing a TLS handshake requires
   signing the transcript in `CertificateVerify` — impossible without the private key.

The public key **identifies**; it does not **authenticate**. All authority lives in the
private half.

### 1.6 Verifying a signature ≠ trusting a certificate

The math check answers one narrow question: *was this signed by the holder of that
private key?* A complete validation also requires:

- the CA is in the client's **trust store** (anyone can self-sign a CA and sign whatever
  they like — the signature verifies fine, it is simply worthless)
- the **chain** builds to a trusted root, each link checked, CA basic-constraints honoured
- **validity dates** current
- **revocation** status (CRL / OCSP / stapling)
- **hostname** matches a SAN
- key usage / EKU permits the intended purpose

And what verification never proves: that whoever handed you the cert owns it.
Certificates are public and copyable. Possession is proved in the handshake, not the
certificate.

---

## Part 2 — TLS: Establishing a Secure Channel

### 2.1 The handshake in plain terms

**Hello.** Client sends the server name, the cipher suites it supports, and a large
random number. Server picks a suite and returns its own random number. Neither side
could have predicted the other's value — this makes the session unrepeatable.

**Key agreement (ephemeral Diffie-Hellman).** The paint analogy is exact in shape:

- Both start from the same public colour, yellow
- You secretly add blue → green; the server secretly adds red → orange
- Green and orange cross the wire in the clear
- You add blue to their orange; they add red to your green
- Both arrive at the identical brown

An eavesdropper sees yellow, green, and orange, and cannot produce brown. Un-mixing is
the hard part. In real terms: client sends `aG`, server sends `bG`, both compute `abG`.

**Authentication.** The server sends its certificate chain; the client validates it.
Then `CertificateVerify`: the server signs a hash of the **entire handshake transcript
so far** with its private key.

This is the step that welds the two halves together:

- The transcript includes the client's random value, so the signature is unique to this
  connection. A replayed recording will not match.
- The transcript includes **both DH shares**. A middleman who substitutes their own
  share must re-sign the altered transcript, which requires the server's private key.
  They do not have it. The connection aborts.

Diffie-Hellman alone is trivially MITM-able. The certificate alone proves nothing about
*this* session. Together they are sound.

### 2.2 Nobody transmits the secret

Each side knows its own private scalar, both public shares, and the derived secret.
Neither ever learns the other's private value — and neither needs to. The secret is
reachable from either direction:

- client: *own private* applied to *server's public share*
- server: *own private* applied to *client's public share*

Two paths, one destination. The eavesdropper is strictly worse off than either
participant: they hold both public shares and **zero** private values. One private value
suffices; none is useless regardless of how many public values you collect.

> **Contrast with the old way.** Legacy RSA key transport had the client choose the
> secret, encrypt it under the server's public key, and ship it. Same endpoint, fatal
> weakness: anyone recording that traffic who later stole the server's private key could
> decrypt it retroactively. Ephemeral DH leaves no encrypted copy of the secret on the
> wire — the private scalars are discarded when the session ends. That is **forward
> secrecy**.

### 2.3 Key derivation runs locally, on both machines

Nothing is transmitted. The client computes its keys; the server computes its own. The
results match because the function is **deterministic** — no internal randomness — and
both sides hold identical inputs by that point:

- the shared secret (each derived it independently)
- both random numbers (exchanged openly in the first two messages)
- a hash of the full handshake transcript

Which algorithm to use was settled in the first two messages, before any secret existed.
The negotiated cipher suite names the encryption cipher, hash, and derivation scheme
together.

The shared secret is never used raw. It is stretched into several keys, each generated
with a distinct **label** baked into the computation — "client-to-server traffic",
"server-to-client traffic", and so on. Both sides use the same labels, so both derive
the same key for each role. Direction assignment then falls out of the roles with no
negotiation.

Folding the transcript hash into derivation means the keys depend on every detail of how
the handshake went. Any tampering yields mismatched keys on the two sides, and the final
verification messages catch it before a byte of application data moves.

### 2.4 Why two keys and not one

You could derive a single key. It would mostly work, and it would introduce a genuinely
dangerous failure mode.

**Nonce reuse.** AEAD ciphers such as AES-GCM take a key *and* a per-record counter, and
the pair must never repeat. With one shared key, both sides count independently from
zero — so the client's record #5 and the server's record #5 collide. That is
catastrophic, not untidy:

- two records under the same key+counter can be XORed to strip the encryption
- GCM's authentication key can be recovered outright, permitting **forgery**

You could patch around it — odd counters one way, even the other — but that is fragile
bookkeeping guarding a cliff edge.

**Reflection.** With one key, a valid record from the server is by construction a valid
record from the client. An attacker could bounce traffic back at its sender and it would
decrypt and authenticate cleanly.

**Key separation.** One key, one job. Derivation is a hash — effectively free — so there
is no cost to generating as many independent keys as there are distinct purposes. TLS
1.3 leans on this heavily: separate keys for handshake traffic, application traffic,
resumption, and key updates, all from one root.

> The general lesson: do not rely on operational discipline to prevent a break you can
> make arithmetically unreachable.

### 2.5 Steady-state traffic

Once the handshake completes, **no key material ever crosses the wire again.**

- Client encrypts requests with the **client→server** key; server decrypts with its copy
- Server encrypts responses with the **server→client** key; client decrypts with its copy

Each record additionally carries:

- an **authentication tag** over the ciphertext — any alteration in transit fails the
  check and tears down the connection
- a **sequence counter** feeding a per-record nonce, so identical plaintexts encrypt
  differently and captured records cannot be replayed; the counter is not secret and
  often not even transmitted, since both sides increment in lockstep

Every subsequent resource on the page reuses the same connection and the same keys; only
the counter advances. One expensive asymmetric handshake bootstraps arbitrarily much
cheap symmetric encryption.

### 2.6 Ordinary HTTPS has no client certificate

Browsing a public website is **one-way** TLS. The server proves who it is; the client
stays anonymous. A public site cannot require every visitor to hold a certificate signed
by an authority it trusts — there would be no way to issue them.

If the site needs to know who you are, that happens **above** TLS, after the tunnel
exists: credentials in a form, then a session cookie or JWT on subsequent requests.
Application layer, not certificate layer.

Mutual TLS is the unusual case, and it appears exactly where the participant set is
small, known, and shares a CA — which brings us to Kubernetes.

---

## Part 3 — Kubernetes: PKI in Practice

### 3.1 The certificate inventory

A `kubeadm` cluster generates its own CA and populates `/etc/kubernetes/pki/`.

| Certificate | Holder | Identity presented |
|---|---|---|
| `admin.conf` | human operator via kubectl | `CN=kubernetes-admin, O=kubeadm:cluster-admins` |
| `controller-manager.conf` | kube-controller-manager | `CN=system:kube-controller-manager` |
| `scheduler.conf` | kube-scheduler | `CN=system:kube-scheduler` |
| `kubelet.conf` | each kubelet | `CN=system:node:<name>, O=system:nodes` |
| `apiserver-kubelet-client` | **API server → kubelet** | `system:kube-apiserver-kubelet-client` |
| `apiserver-etcd-client` | **API server → etcd** | client of etcd's *separate* CA |

Note the last two: the API server is itself a client. Traffic authenticates in both
directions, which is something the browser model never does.

Distinct CAs exist for distinct trust domains — the cluster CA, the etcd CA, and the
front-proxy CA (`--requestheader-client-ca-file`) for API aggregation. Compromise of one
should not imply the others.

### 3.2 How a certificate becomes an identity

Enabled by `--client-ca-file` on kube-apiserver. Without that flag, client certificate
authentication is off entirely. With it, the API server requests a certificate during
the handshake and validates any presented chain against the bundle.

Extraction is blunt:

- **CN** → username
- **O** (repeatable) → group memberships

Two consequences people consistently underestimate:

**There is no user object.** Kubernetes has no `User` resource — none, by design.
Nothing to `kubectl get`, nothing to delete. The certificate *is* the identity, an
assertion the CA makes with no backing record anywhere in the cluster.

**There is no revocation.** The API server ignores CRLs and OCSP entirely. A leaked
client certificate is valid until it expires. The only remedies are stripping the
associated RBAC (which affects everyone holding that identity or group) or rotating the
entire CA.

### 3.3 RBAC lives server-side, not in the kubeconfig

| Kubeconfig (client) | RBAC (etcd) |
|---|---|
| *Who you claim to be* — cert, token, or exec plugin | *What you may do* — Roles and Bindings |
| API endpoint and CA to trust | Evaluated afresh on every request |
| Sits on your laptop | You cannot even read it unless RBAC permits |

Request path:

1. **TLS** — chain validated against `--client-ca-file`
2. **Authentication** — `CN` → username, `O` → groups
3. **Authorization** — RBAC searches bindings matching that username or those groups
4. **Admission**, then the operation

Steps 2 and 3 are fully decoupled. The authenticator hands the authorizer a bare
`(username, groups)` tuple and nothing else — it does not know or care whether the
identity arrived by certificate, ServiceAccount token, or OIDC.

Practical implication: **editing your kubeconfig cannot grant you anything.** To change
your identity you would need to forge a certificate signed by the cluster CA. That is
the actual attack; config edits are not.

RBAC is additive with no deny rules — permissions are the union of every binding
matching your user or any group. `kubectl auth can-i --list` shows the result.

### 3.4 Certificates carry identity, never permissions

The certificate says `O=developers`. It says nothing about what `developers` may do.
Verbs, resources, and namespaces live in RoleBindings in etcd. The certificate is a
claim of membership; the binding attaches capability to that membership.

This split earns its keep, asymmetrically:

| Change | Cost |
|---|---|
| Alter what a group may do | instant, server-side, next request |
| Add someone to a group | **reissue their certificate** |
| Remove someone from a group | **revoke — which is impossible** |

Group membership is frozen at signing time. A one-year certificate reading
`O=developers` says that for a year regardless of employment status. This single
asymmetry is the reason certificates are discouraged for humans.

The proof that privilege is not in the file: delete the ClusterRoleBinding for
`kubeadm:cluster-admins`. Same certificate, same groups, now powerless. Nothing about
the file changed.

### 3.5 What is genuinely hardcoded

Worth separating carefully, because the answer is "much less than people assume."

**Compiled into the API server's Go source — exactly one group:** `system:masters`. The
authorizer short-circuits to *allow* the moment it sees that group, before RBAC is
consulted. It cannot be removed, bound, or restricted.

**Everything else is ordinary RBAC.** `system:nodes`,
`system:kube-controller-manager`, `system:kube-scheduler` are plain strings with plain
ClusterRoleBindings behind them. The API server *bootstraps* a set of default
`system:`-prefixed ClusterRoles and bindings on startup and reconciles them each boot —
but they are real objects:

```bash
kubectl get clusterrolebinding system:kube-controller-manager -o yaml
```

Delete that binding and the controller-manager starts collecting 403s with the same
certificate. Pre-created is not the same as hardcoded.

**The names in the certificates are pure convention.** kubeadm chose
`CN=system:kube-controller-manager` so it would match a binding the API server ships.
Nothing in the code requires it — sign `CN=bob` and create a binding for `bob` and it
works identically.

The one enforcement attached to the naming convention: the API server refuses to
auto-approve CSRs requesting usernames or groups prefixed `system:`, so nobody can
simply ask for a certificate claiming to be `system:masters`.

### 3.6 Why `system:masters` has to be magic

The bypass looks like a wart until you ask what `admin.conf` must survive.

`kubeadm` writes `admin.conf` **before the API server has ever started** — signed
offline from a CA file on disk. There is no cluster yet in which to create anything.
When the API server does come up, the RBAC objects do not exist either; kubeadm uses
`admin.conf` to create them.

So the credential that bootstraps RBAC cannot itself depend on RBAC. Hence a hardcoded
bypass rather than a binding.

The same property is what makes it the break-glass credential. Trace what `admin.conf`
requires end to end:

| Step | Requires etcd? |
|---|---|
| Validate certificate chain | no — pure math against a file |
| Establish identity | no — there is no user object to look up |
| Authorize | no — `system:masters` short-circuits |

**Zero etcd reads.** It works on a half-broken cluster, which is precisely when an
operator needs it most.

That same power is why a leaked `admin.conf` is an emergency rather than an
inconvenience — unrevocable, unrestricted, and typically valid for a year. Newer kubeadm
moved the default to `O=kubeadm:cluster-admins`, a normal group with a normal binding
you can actually strip. The `system:masters` path remains for genuine emergencies.

### 3.7 ServiceAccounts: a different mechanism entirely

No CA is involved anywhere.

| | Client certificate | ServiceAccount token |
|---|---|---|
| Format | X.509 | JWT |
| Signed by | cluster CA | API server's own SA signing key |
| Username | from `CN` | `system:serviceaccount:<ns>:<name>` |
| Groups | from `O` | `system:serviceaccounts`, `system:serviceaccounts:<ns>` |
| An API object? | **no** | **yes** — `kubectl get sa` |
| Revocable? | **no** | **yes** — delete the SA |

That "is it an object" row drives everything else. A certificate identity is an
assertion with nothing behind it. A ServiceAccount is a real object in etcd with a
namespace and a UID — so modern bound tokens are checked against the live object on
every request. Delete the ServiceAccount and its tokens die immediately, unexpired ones
included. Same if the bound pod disappears.

Projected into pods at `/var/run/secrets/kubernetes.io/serviceaccount/`:

- `token` — short-lived (an hour by default), auto-rotated by the kubelet, bound to that
  specific pod
- `ca.crt` — the cluster CA, used only to **verify the API server**, exactly as a
  browser uses its trust store
- `namespace`

The pod therefore holds a CA certificate but has no certificate of its own. It verifies
the server and authenticates with the token. Legacy Secret-based tokens, which never
expired, were the problem bound tokens were introduced to solve.

### 3.8 Pods still get authorized like everyone else

The token authenticates; it does not authorize. Full path for a pod's API call:

1. **TLS** — pod verifies the API server using `ca.crt`; one-way, no client cert
2. **Authentication** — `Authorization: Bearer <token>`; signature checked against the
   SA signing key, expiry checked, SA and pod confirmed to still exist
3. **Authorization** — RBAC, identically to a human request
4. **Admission**, then the operation

The commonly-missed point: the `default` ServiceAccount every pod receives
automatically has **no RBAC bindings at all**. It authenticates perfectly and can do
essentially nothing — `403 Forbidden` on almost everything. Access is granted
explicitly:

```yaml
subjects:
- kind: ServiceAccount
  name: my-app
  namespace: prod
roleRef:
  kind: Role
  name: configmap-reader
```

Two operational notes:

- If a pod never calls the API — most do not — set
  `automountServiceAccountToken: false`. No reason to mount an unused credential.
- Binding anything to the `system:serviceaccounts` group grants it to **every pod in the
  cluster**. A classic way to over-permission everything at once.

### 3.9 Do pods have certificates?

Not by default. Three opt-in routes exist:

1. **Mount your own** — cert and key in a Secret, typically from a public CA, for a pod
   terminating external TLS
2. **cert-manager** — declare a `Certificate` resource; it obtains from Let's Encrypt,
   Vault, or an internal CA, writes to a Secret, and renews automatically
3. **Service mesh** — Istio or Linkerd issue short-lived certs to sidecars, often
   minutes long, held in memory, encoding a SPIFFE identity such as
   `spiffe://cluster.local/ns/prod/sa/my-app`, giving real workload-to-workload mTLS

The CSR API (`certificates.k8s.io`) also permits requesting a cluster-CA-signed
certificate, but signing application certificates with the cluster CA is discouraged —
that CA's signature is what confers control-plane trust.

Default is token-only because a token is revocable, self-rotating, needs no key
management, and covers the one thing every pod might need: talking to the API server.
Certificates solve a different problem — mutual authentication between workloads — which
most clusters do not have until a mesh is added.

---

## Part 4 — The Design Logic

### 4.1 The central trade-off

Certificates and ServiceAccount tokens cannot substitute for one another, and the reason
is a single property viewed from two sides.

| | Certificates | SA tokens |
|---|---|---|
| Verifiable offline? | **yes** | no |
| Requires running etcd to authenticate? | **no** | **yes** |
| Revocable? | no | **yes** |
| Suits ephemeral identities? | no | **yes** |
| Works during bootstrap? | **yes** | no |

Certificates verify offline **because** they are self-contained — which is exactly why
they cannot be revoked. SA tokens are revocable **because** they are checked against
live state — which is exactly why they cannot bootstrap.

**These are the same property.** No single credential can have both. So Kubernetes uses
the self-contained one where nothing is running yet, and the revocable one everywhere
else.

### 4.2 Why ServiceAccounts cannot do the control plane's job

Consider the ordering. The kubelet starts and must reach the API server. But SA tokens
are *issued and validated by the API server*, and bound tokens are checked against the
live SA object in etcd. Using one requires a functioning API server and etcd — the very
things being brought up.

etcd is worse still: the API server authenticates *to* etcd, and etcd has no concept of
a Kubernetes ServiceAccount. It speaks TLS and nothing else.

Certificates work here because verification is arithmetic against a file. No lookups, no
running cluster, no external dependency.

This is also why control-plane components will keep using certificates regardless of
what humans use. OIDC would make cluster startup depend on an external identity provider
being reachable — and if it is not, the control plane cannot start and you cannot log in
to fix it.

### 4.3 Why certificates cannot do a pod's job

Pods are ephemeral and unpredictable: created by a Deployment, scaled to forty,
rescheduled, gone. Per-pod certificates would demand signing on every pod start and
revocation on every pod death. Kubernetes cannot revoke. Within a day a busy cluster
would hold thousands of live credentials for pods that no longer exist, with no way to
kill any of them.

A ServiceAccount is an object, so its token can be checked against reality on every
request — but only because something authoritative is running to be asked.

### 4.4 Why not a ServiceAccount token instead of `admin.conf`?

It works, mechanically — a token in a kubeconfig is a normal pattern for dashboards and
CI. It is nevertheless the wrong default, for three separate reasons.

**Ordering.** `admin.conf` is written before the API server exists. Creating a
ServiceAccount requires a running API server, which requires a credential. Circular.

**Break-glass.** Bound SA tokens are validated against etcd. If etcd is degraded —
precisely when admin access matters most — the lookup fails and the token is worthless.
`admin.conf` needs zero etcd reads (§3.6).

**ServiceAccounts model workloads, not people:**

- **Namespaced** — every human admin becomes an object in some arbitrary namespace
- **No groups** — cannot bind once to a `platform-team`; SA group membership is fixed
  and derived from the namespace
- **No login** — a raw bearer token means no MFA, no session expiry, no IdP
  deprovisioning
- **No provenance** — audit logs show
  `system:serviceaccount:kube-system:admin-user`; if three people share it, you cannot
  tell who did what

An SA token for a human is a lateral move: an unrevocable certificate traded for an
unattributable shared secret.

### 4.5 Why humans should use OIDC

Humans bootstrap nothing, so they never needed the one property certificates uniquely
provide.

| | Client certificate | OIDC token |
|---|---|---|
| Revocation | none — valid to expiry | deprovision at the IdP |
| Lifetime | months to years | minutes |
| Groups | frozen at signing time | refreshed each login |
| Rotation | reissue via the CA | automatic refresh |
| MFA | none | whatever the IdP enforces |
| Audit provenance | a CN string | a real account |

The API server validates OIDC tokens against keys fetched from the provider's JWKS
endpoint — it never contacts the IdP per request. Configured via `--oidc-issuer-url`,
`--oidc-client-id`, `--oidc-username-claim`, `--oidc-groups-claim`.

In kubeconfig, the modern form keeps nothing durable on disk:

```yaml
users:
- name: jane
  user:
    exec:
      apiVersion: client.authentication.k8s.io/v1
      command: kubectl
      args: ["oidc-login", "get-token", "--oidc-issuer-url=..."]
```

kubectl invokes the plugin per call; the token is cached under `~/.kube/cache/`, not
written into the config. This is the mechanism behind `gke-gcloud-auth-plugin`,
`aws eks get-token`, and `kubelogin`.

Managed providers all took this route for humans: **EKS** uses IAM exchanged for a
bearer token; **GKE** removed the client-certificate option entirely; **AKS** offers
Entra ID for user credentials.

RBAC does not change. Whether you arrive as `jane` by certificate or by an OIDC `email`
claim, authorization is identical — which is exactly why the layers stay separable.

### 4.6 Why not simply do what browsers do?

Browser-style means one-way TLS, then a password login, then a session token. That model
needs four things Kubernetes deliberately lacks:

1. **A user database.** There is no `User` object — nowhere to store a password hash, no
   registration, no reset flow. Nothing to log in against.
2. **A login endpoint and sessions.** The API server authenticates every request
   independently and keeps no session state.
3. **A human to type the password.** A kubelet joining at 3am has none. It uses a
   bootstrap token, submits a CSR, receives a signed certificate, and rotates it
   unattended forever.
4. **One-directional trust.** A browser never authenticates *to* you. The API server
   does — to kubelets and to etcd. mTLS is symmetric; the browser model is not.

The instinct is nonetheless correct for humans specifically, and OIDC is precisely that
model retrofitted: log in at an IdP, receive a short-lived token, present it as a bearer
credential.

---

## Summary

**Certificates for machines** — they bootstrap with no dependency on a running cluster,
rotate unattended, and provide mutual authentication in both directions. Their
unrevocability is acceptable for components whose lifecycle you control directly.

**ServiceAccount tokens for workloads** — revocable, self-rotating, bound to a real
object that can be deleted, and suited to identities that appear and vanish constantly.

**OIDC for humans** — the only option offering genuine login, live group membership,
MFA, audit provenance, and revocation on the day someone leaves.

**And one hardcoded bypass**, `system:masters`, because the credential that creates RBAC
cannot itself be governed by RBAC, and because an operator needs a way in when etcd is
sick.

Every one of these choices traces back to the same fault line from Part 1: a signature
is self-contained and verifiable by anyone, anywhere, with no running system to consult
— and that is simultaneously its greatest strength and the reason it can never be taken
back.