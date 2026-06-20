# Build Roadmap

This is the phased plan for turning the current boilerplate into a resume-grade reverse proxy / load balancer.

Each phase builds on the previous one. Do not move to the next phase until the current one is tested and stable.

---

## Phase 0: Single-Backend Proxy

**Goal:** Forward one request to one backend and return the response.

**What to build:**
- Accept a client connection
- Read the raw request bytes
- Open a TCP connection to a hardcoded backend
- Send the request bytes verbatim
- Read the backend response
- Write it back to the client
- Close both connections

**Acceptance criteria:**
- `curl http://localhost:8080/` returns the backend’s response
- Works with a Python `http.server` backend

---

## Phase 1: HTTP/1.1 Correctness

**Goal:** Parse requests and preserve HTTP semantics.

**What to build:**
- Request-line parser (method, path, version)
- Header parser
- Track `Content-Length` and `Transfer-Encoding: chunked`
- Respect `Connection: close` / `keep-alive`
- Build responses with proper status line and headers
- Return `400 Bad Request` on malformed input

**Acceptance criteria:**
- `curl -X POST -d 'data' http://localhost:8080/` forwards the body correctly
- Chunked responses from the backend are streamed to the client
- Keep-alive connections handle multiple requests

---

## Phase 2: Load Balancing

**Goal:** Route traffic across multiple backends.

**What to build:**
- Configurable list of backends
- Round-robin selection
- Least-connections selection
- Track backend state (active connections, healthy flag)

**Acceptance criteria:**
- Two backends receive traffic
- Round-robin alternates evenly
- Least-connections favors the backend with fewer active connections

---

## Phase 3: Connection Pooling + Keep-Alive

**Goal:** Reuse upstream connections.

**What to build:**
- Upstream connection pool per backend
- Borrow/return connections
- Idle timeout and pool size limits
- Client keep-alive support

**Acceptance criteria:**
- Multiple client requests reuse the same backend connection
- Idle connections are closed after timeout

---

## Phase 4: Health Checks

**Goal:** Detect and remove failed backends.

**What to build:**
- Passive health checks: increment failure counter on error/timeout/5xx
- Mark backend unhealthy after threshold
- Active health checks: periodic HTTP/TCP probe
- Recover backend after successful probes

**Acceptance criteria:**
- Killing a backend stops traffic to it within seconds
- Restarting the backend brings it back into rotation

---

## Phase 5: Event-Driven Core

**Goal:** Scale to many concurrent connections.

**What to build:**
- Non-blocking sockets
- Event loop using `kqueue` on macOS and `epoll` on Linux
- Per-connection state machine
- Single-threaded event loop first; optional thread pool later

**Acceptance criteria:**
- Handles 1000+ concurrent connections
- No one-thread-per-connection overhead

---

## Phase 6: Observability + Operations

**Goal:** Make the proxy runnable and debuggable.

**What to build:**
- Structured request logging
- `/metrics` endpoint (request count, latency, backend status)
- Request IDs
- Config file (JSON)
- Signal handling (SIGINT, SIGHUP)
- Graceful config reload without dropping active connections

**Acceptance criteria:**
- Editing `config.json` and sending SIGHUP updates backends
- Logs contain method, path, status, backend, duration

---

## Phase 7: Stretch Goals

Pick any of these after Phase 6:

- **TLS termination** (OpenSSL or similar)
- **Rate limiting** (token bucket per client)
- **Caching layer** (LRU cache for responses)
- **HTTP/2**
- **Weighted least-connections**
- **Canary routing / A-B testing**

---

## Suggested Order of Work

1. Refactor the current `src/main.c` into modules.
2. Implement Phase 0.
3. Write unit tests for the parser.
4. Add load balancing.
5. Add connection pooling.
6. Add health checks.
7. Migrate to event-driven I/O.
8. Add logging, metrics, and config reload.
9. Benchmark and document.
