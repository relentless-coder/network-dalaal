# Reverse Proxy / Load Balancer — Project Overview

## Goals

Build a working reverse proxy and load balancer from scratch in C that can:

- Listen on a port (e.g., 8080)
- Accept HTTP/1.1 client connections
- Parse requests (method, path, headers, body)
- Route each request to one of several backend servers
- Load balance traffic across healthy backends
- Reuse upstream connections via a pool
- Detect and remove unhealthy backends
- Return backend responses to clients
- Log requests and expose metrics
- Reload config without dropping active connections

---

## Learning Objectives

By building this, you'll understand:

- **Sockets programming** — How network communication actually works
- **HTTP/1.1 protocol** — Request/response semantics, keep-alive, chunked encoding
- **System calls** — Working directly with OS APIs
- **Concurrent I/O** — Threading and event-driven models (`epoll` / `kqueue` / `io_uring`)
- **Proxying** — Forwarding traffic while preserving semantics
- **Load balancing** — Algorithms and their tradeoffs
- **Connection management** — Pools, idle timeouts, keep-alive
- **Reliability** — Health checks, retries, timeouts, circuit breakers
- **Observability** — Logging, metrics, tracing
- **Resource management** — File descriptors, memory, connection lifecycles

---

## Project Phases

### Phase 0: Single-Backend Proxy
**Goal:** Forward one request to one backend and return the response.

**Components:**
- Socket setup (bind, listen, accept)
- Read client request
- Establish backend connection
- Forward raw request bytes
- Read backend response
- Send response to client

**Result:** End-to-end proxy works for a single request.

---

### Phase 1: HTTP/1.1 Parser
**Goal:** Parse the request properly and preserve semantics.

**New concepts:**
- Request line parsing (method, path, version)
- Header parsing
- Content-Length and chunked transfer encoding
- Connection: keep-alive / close

**Result:** Proxy can handle arbitrary HTTP/1.1 requests.

---

### Phase 2: Multi-Backend + Load Balancing
**Goal:** Route traffic across multiple backends.

**Approach:** Maintain a list of backends and pick one per request.

**Algorithms:**
- Round-robin
- Least-connections
- Weighted round-robin (optional)
- Consistent hashing (optional)

**Result:** Traffic is distributed across backends.

---

### Phase 3: Connection Pooling + Keep-Alive
**Goal:** Reuse backend connections and support client keep-alive.

**New concepts:**
- Upstream connection pool
- Idle timeouts
- Handling multiple client requests per connection

**Result:** Better throughput and lower latency.

---

### Phase 4: Health Checks
**Goal:** Detect and remove failed backends.

**Approach:**
- Passive: mark backend down after connection errors or 5xx thresholds
- Active: periodic TCP / HTTP health probes

**Result:** Failed backends stop receiving traffic; recovery is automatic.

---

### Phase 5: Event-Driven Core
**Goal:** Scale to thousands of concurrent connections.

**Approach:** Replace blocking per-thread model with an event loop.

**New concepts:**
- `epoll` (Linux) / `kqueue` (macOS/BSD) / `io_uring` (modern Linux)
- Non-blocking I/O
- State machines per connection

**Result:** Production-grade scalability.

---

### Phase 6: Observability + Operations
**Goal:** Make the proxy runnable and debuggable.

**Components:**
- Structured request logging
- `/metrics` endpoint
- Request IDs
- Config file (JSON or custom format)
- Graceful config reload via SIGHUP
- Signal handling

**Result:** The proxy behaves like real infrastructure.

---

### Phase 7: Stretch Goals
**Optional advanced features:**
- TLS termination
- Rate limiting
- Caching
- HTTP/2
- Weighted least-connections

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Reverse Proxy                             │
│                                                                  │
│  ┌──────────┐    ┌──────────────┐    ┌──────────────────────┐   │
│  │ Listener │───→│ HTTP Parser  │───→│ Router / Load        │   │
│  │          │    │              │    │ Balancer             │   │
│  └──────────┘    └──────────────┘    └──────────────────────┘   │
│                                               │                  │
│                                               ↓                  │
│                              ┌────────────────────────┐         │
│                              │ Upstream Connection    │         │
│                              │ Pool                   │         │
│                              └────────────────────────┘         │
│                                               │                  │
│                                               ↓                  │
│                              ┌────────────────────────┐         │
│                              │ Backend Servers        │         │
│                              └────────────────────────┘         │
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │
│  │ Health Check │    │    Logger    │    │   Metrics    │       │
│  └──────────────┘    └──────────────┘    └──────────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

```
1. Client connects to proxy
   ↓
2. Proxy accepts connection
   ↓
3. Proxy reads and parses HTTP request
   ↓
4. Router selects a healthy backend
   ↓
5. Proxy gets (or creates) upstream connection
   ↓
6. Proxy forwards request to backend
   ↓
7. Proxy reads backend response
   ↓
8. Proxy sends response to client
   ↓
9. Connection is kept alive or closed
```

---

## Key Data Structures

### HTTP Request
```c
typedef struct {
    char method[16];          // "GET", "POST", etc.
    char path[256];           // "/hello"
    char version[16];         // "HTTP/1.1"
    // Headers, body, keep-alive flag
} http_request_t;
```

### Backend
```c
typedef struct {
    char host[64];            // "127.0.0.1"
    int port;                 // 9001
    int healthy;              // 1 = up, 0 = down
    int active_connections;   // For least-connections
    int failure_count;        // For passive health checks
} backend_t;
```

### Upstream Connection
```c
typedef struct {
    int fd;
    backend_t *backend;
    int in_use;
    time_t last_used;
} upstream_conn_t;
```

### Proxy Context
```c
typedef struct {
    int listen_fd;
    backend_t *backends;
    int backend_count;
    upstream_conn_t *conn_pool;
    int load_balancing_strategy;
} proxy_ctx_t;
```

---

## File Structure (Suggested)

```
http-server/
├── docs/
│   ├── 00-project-overview.md
│   ├── 01-sockets-basics.md
│   ├── 02-file-descriptors.md
│   ├── 03-kernel-buffers.md
│   ├── 04-http-protocol.md
│   ├── 05-getting-started.md
│   └── 06-roadmap.md
├── src/
│   ├── main.c           # Entry point and event loop
│   ├── listener.c       # Accept client connections
│   ├── parser.c         # HTTP/1.1 request parser
│   ├── router.c         # Route selection / load balancing
│   ├── upstream.c       # Backend connection pool
│   ├── health.c         # Health checks
│   ├── config.c         # Config parsing
│   ├── metrics.c        # Logging and metrics
│   └── util.c           # Helpers
├── include/
│   ├── listener.h
│   ├── parser.h
│   ├── router.h
│   ├── upstream.h
│   ├── health.h
│   ├── config.h
│   └── metrics.h
├── tests/
│   └── test_parser.c
├── scripts/
│   └── bench.sh
├── Makefile
├── config.json
└── README.md
```

---

## Testing Strategy

### Manual Testing
```bash
# Start backends
python3 -m http.server 9001 &
python3 -m http.server 9002 &

# Start proxy
./proxy config.json

# Send requests
curl http://localhost:8080/
curl http://localhost:8080/hello
curl -v http://localhost:8080/
```

### Kill a Backend
```bash
kill <backend-pid>
# Proxy should stop sending traffic to it
```

### Benchmarking
```bash
# Install wrk or hey
wrk -t4 -c400 -d30s http://localhost:8080/
```

---

## Success Criteria

**Phase 0 complete when:**
- ✅ Proxy listens on port 8080
- ✅ Accepts a client connection
- ✅ Forwards the request to one backend
- ✅ Returns the backend response to the client

**Phase 1 complete when:**
- ✅ Parses method, path, version, and headers
- ✅ Handles keep-alive correctly
- ✅ Returns 400 on malformed requests

**Phase 2 complete when:**
- ✅ Multiple backends configured
- ✅ Round-robin load balancing works
- ✅ Least-connections load balancing works

**Phase 3 complete when:**
- ✅ Upstream connections are reused
- ✅ Idle connections time out
- ✅ Client keep-alive is supported

**Phase 4 complete when:**
- ✅ Failed backends are marked unhealthy
- ✅ Recovery is detected
- ✅ Traffic stops routing to unhealthy backends

**Phase 5 complete when:**
- ✅ Event loop handles many concurrent connections
- ✅ No one-thread-per-connection bottleneck

**Phase 6 complete when:**
- ✅ Config file controls backends and strategy
- ✅ Logs every request
- ✅ `/metrics` returns useful stats
- ✅ Config reload works without dropping connections

---

## Next Steps After MVP

1. Add TLS termination
2. Implement rate limiting
3. Add a cache layer
4. Benchmark against nginx/Envoy
5. Write a design doc for interviews

---

## Resources

- Beej's Guide to Network Programming: https://beej.us/guide/bgnet/
- RFC 7230 (HTTP/1.1): https://tools.ietf.org/html/rfc7230
- Unix socket man pages: `man 2 socket`, `man 2 bind`, `man 2 listen`, `man 2 accept`
- nginx architecture: https://www.nginx.com/blog/inside-nginx-how-we-designed-for-performance-scale/
- Your own documentation in this `docs/` folder!
