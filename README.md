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

## Architecture

```
┌─────────┐      ┌──────────────┐      ┌──────────────────────┐      ┌──────────┐
│ Client  │─────→│   Listener   │─────→│   HTTP Parser /      │─────→│ Upstream │
└─────────┘      │  (TCP socket)│      │   Request Router     │      │ Connection
                 └──────────────┘      └──────────────────────┘      │   Pool   │
                            │                      │                 └────┬─────┘
                            │                      │                      │
                            ↓                      ↓                      ↓
                    ┌──────────────┐      ┌─────────────────┐      ┌──────────┐
                    │ Load Balancer│      │ Health Checker  │      │ Backend  │
                    │ (algorithms) │      │ (passive/active)│      │ Servers  │
                    └──────────────┘      └─────────────────┘      └──────────┘
```

## Project Structure

```
http-server/
├── docs/                 # Design docs and study notes
│   ├── 00-project-overview.md
│   ├── 01-sockets-basics.md
│   ├── 02-file-descriptors.md
│   ├── 03-kernel-buffers.md
│   ├── 04-http-protocol.md
│   ├── 05-getting-started.md
│   └── 06-roadmap.md
├── src/                  # Source code
│   ├── main.c            # Entry point and event loop
│   ├── listener.c        # Accept client connections
│   ├── parser.c          # HTTP/1.1 request parser
│   ├── router.c          # Route selection / load balancing
│   ├── upstream.c        # Backend connection pool
│   ├── health.c          # Health checks
│   ├── config.c          # Config file parsing
│   ├── metrics.c         # Metrics and logging
│   └── util.c            # Helpers
├── include/              # Headers
├── tests/                # Unit and integration tests
├── scripts/              # Benchmark / test helpers
├── Makefile
└── README.md
```

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

See [`docs/06-roadmap.md`](docs/06-roadmap.md) for the full phased plan.

High-level milestones:

1. **MVP proxy** — forward a request to one backend and return the response ✅
2. **Basic load balancing** — round-robin across multiple backends ✅
3. **Config + signals** — config file, SIGINT/SIGHUP handling
4. **HTTP/1.1 correctness** — keep-alive, chunked encoding, Content-Length, headers
5. **Connection pooling** — reuse upstream connections, idle timeouts
6. **Advanced load balancing** — least-connections, weighted round-robin
7. **Health checks** — passive failure detection + active probes
8. **Event-driven core** — replace blocking model with `epoll`/`kqueue`/`io_uring`
9. **Observability** — request logs, metrics endpoint, request IDs
10. **Operations** — graceful config reload, robust signal handling
11. **Stretch goals** — TLS termination, rate limiting, caching

## Why this project

This is a senior-engineer-level systems project. It forces you to reason about:

- Network protocols and system calls
- Concurrent connection management
- Memory safety and resource lifecycles in C
- Reliability, failure detection, and backpressure
- Observability and operations

It also gives you a concrete story for technical interviews.
