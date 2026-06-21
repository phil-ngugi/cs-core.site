# How Database Connections Work (and How to Build a Driver in Node.js)

When you call `client.connect()` or `pool.query(...)`, it feels like one operation. Underneath, a database driver is doing several distinct jobs:

1. Opening a **TCP socket** to the server
2. Performing a **handshake** (and often authentication)
3. Speaking the database's **wire protocol** — a specific byte format for requests and replies
4. **Correlating** requests with responses on that socket
5. Managing the **connection lifecycle** (timeouts, reconnects, pooling)

A "driver" is just the code that does steps 1–5 so your application can call `.query()` instead of writing bytes to a socket. To make this concrete, we'll build a real (if minimal) driver from scratch — for Redis, using its RESP protocol, because it's simple enough to implement completely in one file while still teaching every concept that scales up to Postgres, MySQL, or MongoDB drivers.

---

## 1. A connection is just a socket

There's no special "database channel." A DB connection is a TCP socket like any other:

```js
const net = require('net');
const socket = net.createConnection({ host: '127.0.0.1', port: 6379 });
```

Once connected, the socket is a raw, bidirectional stream of bytes. Nothing about it knows what "SQL" or "SET key value" means yet — that meaning is added by the protocol layer on top.

```
┌─────────────┐        TCP socket (bytes)        ┌─────────────┐
│  Your app   │ ───────────────────────────────► │   Database   │
│  + driver   │ ◄─────────────────────────────── │    server    │
└─────────────┘                                   └─────────────┘
```

## 2. The handshake

Most databases don't let you send queries the instant the socket opens. There's usually a startup exchange first:

- **Postgres**: client sends a `StartupMessage` with the username/database name; server may request a password (`AuthenticationCleartextPassword`, `AuthenticationMD5Password`, or SCRAM for newer versions); client replies; server sends `ReadyForQuery`.
- **MySQL**: server speaks first, sending a handshake packet with its version and a salt; client replies with credentials hashed against that salt.
- **Redis**: no mandatory handshake — you can send commands immediately (optionally `AUTH` first if a password is set).

This is why Redis is a good teaching example: it lets us skip straight to the interesting part — encoding and decoding the protocol — without building a full SASL/SCRAM negotiation first.

## 3. The wire protocol

This is the actual contract: what bytes mean what. Redis uses **RESP** (REdis Serialization Protocol), a small text-based format:

| Prefix | Type | Example |
|---|---|---|
| `+` | Simple string | `+OK\r\n` |
| `-` | Error | `-ERR unknown command\r\n` |
| `:` | Integer | `:1000\r\n` |
| `$` | Bulk string | `$5\r\nhello\r\n` (or `$-1\r\n` for null) |
| `*` | Array | `*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n` |

A command like `GET foo` is sent as an **array of bulk strings**:

```
*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n
```

Postgres and MySQL use binary, more complex variants of the same idea — length-prefixed messages with a type byte — but the core pattern is identical: *frame your data so the reader knows where one message ends and the next begins.*

## 4. Building the driver

### 4.1 Encoding commands

```js
function encodeCommand(args) {
  let out = `*${args.length}\r\n`;
  for (const arg of args) {
    const str = String(arg);
    out += `$${Buffer.byteLength(str)}\r\n${str}\r\n`;
  }
  return Buffer.from(out, 'utf8');
}

// encodeCommand(['SET', 'greeting', 'hello world'])
// => "*3\r\n$3\r\nSET\r\n$8\r\ngreeting\r\n$11\r\nhello world\r\n"
```

### 4.2 Decoding replies — the part everyone gets wrong first

TCP doesn't preserve message boundaries. A reply might arrive in one `data` event, split across two, or batched with the *next* reply. A driver has to buffer bytes and only emit a parsed value once it has a complete message:

```js
class RespParser {
  constructor(onReply) {
    this.buffer = Buffer.alloc(0);
    this.onReply = onReply;
  }

  feed(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    let result;
    while ((result = this._tryParse(this.buffer)) !== null) {
      const [value, bytesConsumed] = result;
      this.buffer = this.buffer.subarray(bytesConsumed);
      this.onReply(value);
    }
  }

  // Returns [parsedValue, bytesConsumed] or null if more data is needed.
  _tryParse(buf) {
    if (buf.length === 0) return null;
    const type = String.fromCharCode(buf[0]);
    const lineEnd = buf.indexOf('\r\n');
    if (lineEnd === -1) return null; // incomplete line, wait for more bytes

    switch (type) {
      case '+': // simple string
        return [buf.toString('utf8', 1, lineEnd), lineEnd + 2];

      case '-': // error
        return [new Error(buf.toString('utf8', 1, lineEnd)), lineEnd + 2];

      case ':': // integer
        return [parseInt(buf.toString('utf8', 1, lineEnd), 10), lineEnd + 2];

      case '$': { // bulk string
        const len = parseInt(buf.toString('utf8', 1, lineEnd), 10);
        if (len === -1) return [null, lineEnd + 2]; // nil
        const dataStart = lineEnd + 2;
        const dataEnd = dataStart + len;
        if (buf.length < dataEnd + 2) return null; // body not fully arrived yet
        return [buf.toString('utf8', dataStart, dataEnd), dataEnd + 2];
      }

      case '*': { // array
        const count = parseInt(buf.toString('utf8', 1, lineEnd), 10);
        if (count === -1) return [null, lineEnd + 2];
        let offset = lineEnd + 2;
        const items = [];
        for (let i = 0; i < count; i++) {
          const sub = this._tryParse(buf.subarray(offset));
          if (sub === null) return null; // wait for the rest of the array
          items.push(sub[0]);
          offset += sub[1];
        }
        return [items, offset];
      }

      default:
        throw new Error(`Unknown RESP type byte: ${type}`);
    }
  }
}
```

The key idea: `_tryParse` either returns a complete value *and* how many bytes it consumed, or `null` meaning "not enough data yet — try again after the next chunk." This pattern (incremental, resumable parsing over a byte buffer) is how essentially every binary protocol parser works, from HTTP/2 frames to Postgres's message format.

### 4.3 Correlating requests with replies

A single TCP connection can have several commands "in flight" if you don't wait for each reply before sending the next (this is called **pipelining**). RESP guarantees replies come back in the *same order* commands were sent, so a simple FIFO queue of pending promises is enough to match them up correctly:

```js
class RedisConnection {
  constructor({ host = '127.0.0.1', port = 6379 } = {}) {
    this.host = host;
    this.port = port;
    this.queue = []; // pending { resolve, reject }, in send order
    this.socket = null;
  }

  connect() {
    return new Promise((resolve, reject) => {
      this.socket = net.createConnection({ host: this.host, port: this.port });

      this.parser = new RespParser((reply) => {
        const pending = this.queue.shift();
        if (!pending) return; // unsolicited reply — shouldn't happen with RESP, but be safe
        if (reply instanceof Error) pending.reject(reply);
        else pending.resolve(reply);
      });

      this.socket.once('connect', resolve);
      this.socket.once('error', reject);
      this.socket.on('data', (chunk) => this.parser.feed(chunk));
      this.socket.on('close', () => {
        while (this.queue.length) {
          this.queue.shift().reject(new Error('Connection closed'));
        }
      });
    });
  }

  command(...args) {
    return new Promise((resolve, reject) => {
      this.queue.push({ resolve, reject });
      this.socket.write(encodeCommand(args));
    });
  }

  close() {
    return new Promise((resolve) => this.socket.end(resolve));
  }
}
```

### 4.4 Using it

```js
(async () => {
  const conn = new RedisConnection({ host: '127.0.0.1', port: 6379 });
  await conn.connect();

  await conn.command('SET', 'greeting', 'hello world');
  const value = await conn.command('GET', 'greeting');
  console.log(value); // "hello world"

  await conn.close();
})();
```

That's a working, real driver in about 80 lines — it correctly handles partial TCP reads, pipelined commands, and connection teardown.

## 5. Connection pooling

Opening a TCP connection (plus auth handshake) is relatively expensive — tens of milliseconds. Real applications handle many concurrent requests, so creating a fresh connection per request is wasteful and slow. A **pool** keeps a set of already-connected sockets ready to reuse:

```js
class Pool {
  constructor(factory, { max = 10 } = {}) {
    this.factory = factory; // () => new RedisConnection(...)
    this.max = max;
    this.free = [];
    this.size = 0;
    this.waiters = [];
  }

  async acquire() {
    if (this.free.length > 0) return this.free.pop();

    if (this.size < this.max) {
      this.size++;
      const conn = this.factory();
      await conn.connect();
      return conn;
    }

    // pool is full — wait for someone to release a connection
    return new Promise((resolve) => this.waiters.push(resolve));
  }

  release(conn) {
    if (this.waiters.length > 0) {
      this.waiters.shift()(conn);
    } else {
      this.free.push(conn);
    }
  }
}
```

Usage:

```js
const pool = new Pool(() => new RedisConnection({ host: '127.0.0.1', port: 6379 }), { max: 5 });

async function getGreeting() {
  const conn = await pool.acquire();
  try {
    return await conn.command('GET', 'greeting');
  } finally {
    pool.release(conn);
  }
}
```

This is the same shape used by `pg.Pool`, `mysql2`'s pool, and `ioredis`'s cluster client — acquire, use, release (ideally in a `finally` so a thrown error doesn't leak the connection).

## 6. What real drivers add on top of this

The toy driver above covers the core loop. Production drivers (`pg`, `mysql2`, `ioredis`, `mongodb`) layer on:

- **TLS** — wrapping the socket in `tls.connect()` instead of `net.createConnection()`.
- **Authentication negotiation** — SCRAM-SHA-256 for Postgres, native MySQL auth plugins, Redis `AUTH`/`HELLO`.
- **Backpressure** — checking the return value of `socket.write()` and pausing further writes until `'drain'` fires, so a slow consumer doesn't cause unbounded memory growth.
- **Per-command timeouts** — rejecting a pending promise if no reply arrives in time, separate from the OS-level TCP timeout.
- **Reconnection with backoff** — detecting `close`/`error`, then retrying with increasing delay rather than hammering a server that's restarting.
- **Prepared statements / extended query protocol** — Postgres, for example, splits a query into separate `Parse`, `Bind`, `Execute` messages so parameters are sent as data, not interpolated into the query text. This is what makes parameterized queries (`$1`, `?`) immune to SQL injection — the driver, not string concatenation, keeps code and data separate.
- **Multiplexing vs. strict ordering** — RESP and the simple Postgres protocol are strictly ordered (replies match request order), but some protocols (e.g., MongoDB's wire protocol) tag each request with an ID so replies can arrive out of order over one connection.
- **Health checks** — periodically pinging idle pooled connections so a pool doesn't hand out a connection the server silently dropped.

## 7. Summary

| Concept | What it means | Where you saw it above |
|---|---|---|
| Socket | Raw TCP byte stream | `net.createConnection` |
| Handshake | Initial negotiation before queries are allowed | Skipped for Redis, present in Postgres/MySQL |
| Wire protocol | Byte format for requests/replies | RESP encode/decode functions |
| Framing | Knowing where one message ends and the next begins | `RespParser._tryParse` returning `null` on incomplete data |
| Correlation | Matching replies to the request that caused them | FIFO `queue` of pending promises |
| Pooling | Reusing expensive-to-create connections | `Pool` class |

Every database driver, regardless of language or protocol, is a variation on this same shape. Once you've written one from scratch, reading the source of `pg` or `mysql2` stops looking like magic — it's the same five pieces, with more edge cases handled.