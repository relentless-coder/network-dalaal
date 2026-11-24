# C Reverse Proxy / Load Balancer

A from-scratch reverse proxy and load balancer written in C.

The goal is to understand how edge proxies like nginx and Envoy work at the network layer: HTTP parsing, upstream connection management, load balancing, health checks, and observability.

## What it does

- Accepts HTTP/1.1 client connections
- Parses requests and forwards them to backend servers
- Distributes traffic across backends with pluggable load-balancing algorithms
- Reuses upstream connections via a connection pool
- Monitors backend health and removes failed backends
- Exposes metrics and request logs
- Supports graceful config reload

## Build

```bash
cd /Users/darlingsuno/workspace/c/projects/http-server
make
./proxy config.json
```

## Quick Test

```bash
# Terminal 1: start a simple backend
python3 -m http.server 9001 &
python3 -m http.server 9002 &

# Terminal 2: start the proxy
./proxy config.json

# Terminal 3: send traffic through the proxy
curl http://localhost:8080/
```

## Roadmap

High-level milestones:

1. **MVP proxy** — forward a request to one backend and return the response ✅
2. **Basic load balancing** — round-robin across multiple backends ✅
3. **Config + signals** — config file, SIGINT/SIGHUP handling --skipped--
4. **HTTP/1.1 correctness** — keep-alive, chunked encoding, Content-Length, headers --in progress--
5. **Connection pooling** — reuse upstream connections, idle timeouts
6. **Advanced load balancing** — least-connections, weighted round-robin
7. **Health checks** — passive failure detection + active probes
8. **Event-driven core** — replace blocking model with cross-platform `epoll`/`kqueue` event loop
9. **Multi-reactor threading** — scale across CPU cores with per-thread event loops
10. **Observability** — request logs, metrics endpoint, request IDs
11. **Operations** — graceful config reload, robust signal handling
12. **Stretch goals** — TLS termination, rate limiting, caching
