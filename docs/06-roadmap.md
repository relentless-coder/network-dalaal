# Build Roadmap

This is the phased plan for turning the current boilerplate into a resume-grade reverse proxy / load balancer.

Each phase builds on the previous one. Do not move to the next phase until the current one is tested and stable.

---

## Phase 0: Single-Backend Proxy

**Status:** ✅ Complete.

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

## Phase 1: Basic Load Balancing

**Status:** ✅ Complete.

**Goal:** Route traffic across multiple backends with simple round-robin.

**What to build:**
- Configurable list of backends (hardcoded is fine for this phase)
- Round-robin selection
- Skip unreachable backends
- Return `502 Bad Gateway` when all backends fail

**Acceptance criteria:**
- Three backends receive traffic in round-robin order
- Killing one backend stops traffic to it
- Killing all backends returns `502`

---

## Phase 2: Config File + Signal Handling

**Goal:** Make the proxy configurable without recompiling, and handle OS signals cleanly.

**What to build:**
- JSON config file with `listen` address and `backends` list
- Parse config at startup
- Handle `SIGINT` for clean shutdown
- Handle `SIGTERM` for clean shutdown

**Acceptance criteria:**
- `./proxy config.json` starts with configured backends
- `Ctrl-C` closes listening socket and exits gracefully
- Invalid config prints a useful error and exits non-zero

---

## Phase 3: HTTP/1.1 Correctness

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

## Phase 4: Connection Pooling + Keep-Alive

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

## Phase 5: Advanced Load Balancing

**Goal:** Support algorithms that need per-backend state.

**What to build:**
- Least-connections selection
- Weighted round-robin selection
- Track active connection counts per backend

**Acceptance criteria:**
- Least-connections favors the backend with fewer active connections
- Weighted round-robin distributes according to weights

---

## Phase 6: Health Checks

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

## Phase 7: Event-Driven Core

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

## Phase 8: Observability

**Goal:** Make the proxy debuggable and measurable.

**What to build:**
- Structured request logging
- `/metrics` endpoint (request count, latency, backend status)
- Request IDs

**Acceptance criteria:**
- Logs contain method, path, status, backend, duration
- `/metrics` returns backend health and request counts

---

## Phase 9: Operations

**Goal:** Make the proxy runnable in production-like environments.

**What to build:**
- Graceful config reload without dropping active connections
- Robust signal handling (`SIGINT`, `SIGTERM`, `SIGHUP`)
- Daemonization / systemd unit (optional)

**Acceptance criteria:**
- Editing `config.json` and sending `SIGHUP` updates backends
- Active connections finish before old config is torn down

---

## Phase 10: Stretch Goals

Pick any of these after Phase 9:

- **TLS termination** (OpenSSL or similar)
- **Rate limiting** (token bucket per client)
- **Caching layer** (LRU cache for responses)
- **HTTP/2**
- **Weighted least-connections**
- **Canary routing / A-B testing**

---

## Suggested Order of Work

1. Refactor the current `src/main.c` into modules.
2. Add a JSON config file and signal handling.
3. Implement HTTP/1.1 request/response parsing.
4. Add connection pooling and keep-alive.
5. Add least-connections and weighted round-robin.
6. Add passive and active health checks.
7. Migrate to event-driven I/O.
8. Add logging, metrics, and request IDs.
9. Implement graceful config reload.
10. Benchmark and document.
