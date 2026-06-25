# Current Status — Reverse Proxy / Load Balancer

Last updated: 2026-06-22

## What Works Now

A multi-backend, single-threaded, blocking TCP proxy in `src/main.c`:

- Listens on `0.0.0.0:8080`
- Accepts one client connection at a time
- Reads the raw request into a 4096-byte buffer
- Selects a backend via round-robin from a hardcoded pool
- Skips unreachable backends and continues to the next one
- Forwards the request and streams the backend response back to the client
- Returns `502 Bad Gateway` if all backends are unreachable
- Uses a single `cleanup:` label with `goto` for centralized resource cleanup
- Tested successfully against Python backends on ports 9001, 9002, and 9003

## Current Code Shape

Everything is in one file (`src/main.c`). The loop is:

```
accept() → read() → select_backend() → write() → read-loop() → write-loop() → cleanup
```

Cleanup closes `backend_fd` (if opened) and `client_fd` in one place.

## Recent Lessons / Bugs Fixed

- `inet_pton()` must write to `&backend_address.sin_addr`, not `sin_family`.
- `connect()` returns `0` on success and `-1` on failure — not a positive/negative polarity.
- `write(client_fd, response, read_response)` must use the actual byte count read, not `sizeof(response)`.
- A single `read()` on the backend socket may not capture the full HTTP/1.0 response; stream until EOF.
- `goto cleanup` is an idiomatic C pattern for centralized resource cleanup; it is not the same as "goto considered harmful" in higher-level languages.
- Round-robin `next_index` must advance even when the selected backend is dead, otherwise the selector gets stuck.

## Design Decisions in Progress

### Load Balancing: Round-Robin

- `connect_backend(struct backend *b)` is a pure helper: it creates a socket, attempts `connect()`, and returns the fd or `-1`. It does not modify backend state.
- `select_backend(struct backend_pool *pool, struct backend **out_backend)`:
  - Tries up to `count` backends.
  - Picks the next backend via round-robin.
  - Advances `next_index` on every attempt, regardless of success.
  - On success: returns the connected fd and writes the chosen backend pointer through `out_backend`.
  - On total failure: returns `-1`.

### Backend State

```c
struct backend {
    const char *ip;
    int port;
    int alive;
};

struct backend_pool {
    struct backend *backends;
    int count;
    int next_index;
};
```

- The `alive` field is currently set but not read. It will be used properly once health checks are implemented in Phase 6.

### Single-File vs. Multi-File

- Code currently stays in `src/main.c`.
- Next step is refactoring into modules: listener, router/upstream, config, etc.

## Known Limitations

- Only handles one client at a time (blocking loop, no `epoll`/`kqueue` yet).
- Backends are hardcoded; no config file yet.
- Reads the request once with a fixed 4096-byte buffer; large requests are truncated.
- Does not handle partial writes.
- Does not parse HTTP; just forwards raw bytes.
- Does not support keep-alive or connection reuse.

## Next Steps

1. Refactor `src/main.c` into modules.
2. Add a JSON config file and signal handling (Phase 2).
3. Implement HTTP/1.1 request/response parsing (Phase 3).
4. Add connection pooling and keep-alive (Phase 4).
5. Add least-connections and weighted round-robin (Phase 5).
6. Add health checks (Phase 6).
7. Migrate to event-driven I/O (Phase 7).
