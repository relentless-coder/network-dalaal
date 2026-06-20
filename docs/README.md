# Project Documentation

This folder contains design docs and study notes for the reverse proxy / load balancer project.

## Docs Index

| Doc | Purpose |
|---|---|
| [`00-project-overview.md`](00-project-overview.md) | Current project goals, architecture, phases, and success criteria |
| [`01-sockets-basics.md`](01-sockets-basics.md) | Socket fundamentals |
| [`02-file-descriptors.md`](02-file-descriptors.md) | File descriptors and "everything is a file" |
| [`03-kernel-buffers.md`](03-kernel-buffers.md) | How `read()` and `write()` actually work |
| [`04-http-protocol.md`](04-http-protocol.md) | HTTP request/response format |
| [`05-getting-started.md`](05-getting-started.md) | Original minimal HTTP server tutorial (still useful background) |
| [`06-roadmap.md`](06-roadmap.md) | Phased build plan and milestones |

## Quick Reference

### Socket Workflow
```c
socket()  → Create socket
bind()    → Assign address/port
listen()  → Start accepting connections
accept()  → Get client connection
read()    → Receive data
write()   → Send data
close()   → Clean up
```

### HTTP Request Example
```
GET /hello HTTP/1.1\r\n
Host: localhost:8080\r\n
\r\n
```

### HTTP Response Example
```
HTTP/1.1 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 13\r\n
\r\n
Hello, World!
```

### Proxy Data Flow
```
Client → Listener → HTTP Parser → Load Balancer → Upstream Pool → Backend
```
