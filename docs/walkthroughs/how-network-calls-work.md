# Network Request Journey
*From a process needing data, down to receiving it*

---

## 1. DNS — Finding the Server's IP

- Process calls `getaddrinfo("google.com")`
- libc, running inside the process, checks its local DNS cache
- On miss, opens a UDP socket and sends a DNS query to the local resolver (e.g. `127.0.0.53`)
- Kernel moves the UDP packet to the resolver process (`systemd-resolved`)
- Resolver checks its heap cache — on miss sends UDP query upstream toward `8.8.8.8`
- Response travels back, kernel delivers it to the resolver's socket
- Resolver returns the IP to libc via the same UDP socket
- `getaddrinfo()` returns the IP address to the process

---

## 2. TCP Handshake — Establishing the Connection

- Process calls `connect(fd, server_ip, port)`
- Kernel creates a `struct sock`, picks a random ephemeral port (e.g. `49182`), fills in the four-tuple: `your_ip:49182 ↔ server_ip:443`
- Registers the four-tuple in the global socket hash table
- Kernel sends SYN packet (seq=1000) — down through IP layer, Ethernet layer, out the NIC via DMA onto the wire
- Process thread goes to sleep, blocked on `connect()`
- Server's NIC receives packet, raises interrupt, kernel softirq runs, strips Ethernet header, strips IP header, reads TCP flags — sees SYN
- Server kernel creates its own `struct sock`, picks seq=5000, sends SYN-ACK (seq=5000, ack=1001)
- Your NIC receives SYN-ACK, interrupt fires, softirq runs, kernel looks up four-tuple in hash table, finds your `struct sock`
- Kernel sends ACK (ack=5001), marks socket `ESTABLISHED`
- Wakes your sleeping process — `connect()` returns

---

## 3. TLS Handshake — Encrypting the Connection

- Your process (via a linked TLS library e.g. OpenSSL) sends a `ClientHello` via `send()`
- Server responds with its certificate and a `ServerHello`
- Both sides derive shared encryption keys
- All subsequent data is encrypted — the kernel has no idea, it just sees bytes
- Adds 1-2 more round trips on top of the TCP handshake

---

## 4. Sending the Request

- Process calls `send(fd, "GET / HTTP/1.1...", ...)`
- Kernel copies bytes from userspace buffer into `sk_write_queue` (send buffer) inside `struct sock`
- TCP layer wraps bytes in a segment — adds sequence number, ACK flag, port numbers
- IP layer adds IP header — source and destination IP
- Ethernet layer adds MAC addresses
- NIC picks up the packet via DMA, puts it on the wire
- Kernel keeps bytes in the send buffer until ACK'd — may need to retransmit

---

## 5. Data Travels Across the Network

- Packet hops through routers — each reads the IP destination, looks up its routing table, forwards to the next hop
- Each hop is a kernel doing the same IP layer processing on a different machine
- Eventually reaches the server's NIC

---

## 6. Server Processes the Request

- Server NIC raises interrupt, DMA copies packet into rx ring buffer
- Kernel softirq runs — strips Ethernet header (ethertype `0x0800` → IPv4)
- IP layer strips IP header (protocol field `0x06` → TCP)
- TCP layer reads destination port `443`, looks up hash table, finds the server process's socket
- Copies data into that socket's receive buffer, wakes the server process
- Server process (nginx etc.) calls `recv()`, gets the HTTP request bytes
- Server generates HTTP response, calls `send()`
- Server kernel sends ACK for your request, piggybacked on the response data

---

## 7. Response Travels Back

- Response packets arrive at your NIC
- Interrupt fires, softirq runs, same header-stripping process
- TCP layer reads destination port `49182`, looks up hash table
- Finds your `struct sock` — the same one created during `connect()`
- Copies response bytes into `sk_receive_queue` (receive buffer) inside your `struct sock`
- Your kernel sends ACK back to server — segment confirmed received, server discards it from its send buffer
- Your process is woken up

---

## 8. Data Reaches Your Process

- Your process calls `recv(fd, buf, size)`
- Kernel copies bytes from `sk_receive_queue` into your userspace `buf`
- Returns number of bytes copied
- Repeats for each chunk until the full response is received
- Browser reassembles chunks, parses HTML, discovers it needs more assets
- Reuses the same TCP connection for subsequent requests — no new handshake

---

## 9. Connection Lifecycle After

- Connection sits idle in the browser's connection pool
- Either server closes it after its keepalive timeout (e.g. 65s)
- Or browser closes it after its own idle timeout
- Four-way FIN teardown — both sides exchange FIN + ACK
- Socket enters `TIME_WAIT` for ~60 seconds
- `struct sock` freed from kernel memory, ephemeral port released

---

## Layer Summary

| What                        | Where                              |
|-----------------------------|------------------------------------|
| `getaddrinfo` / HTTP parsing | Userspace — your process          |
| TLS                         | Userspace — linked library         |
| TCP / UDP / IP              | Kernel network stack               |
| Ethernet framing            | Kernel driver + NIC                |
| Physical signals            | NIC hardware                       |

> At no point does the kernel understand HTTP, DNS responses, or TLS.
> It moves bytes between sockets. Everything meaningful is built in userspace on top of that primitive.