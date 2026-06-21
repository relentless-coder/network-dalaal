# Current Status — Reverse Proxy / Load Balancer

Last updated: 2026-06-21

## What Works Now

A single-backend, blocking TCP proxy in `src/main.c`:

- Listens on `0.0.0.0:8080`
- Accepts one client connection at a time
- Reads the raw request into a 4096-byte buffer
- Connects to a hardcoded backend at `127.0.0.1:9001`
- Forwards the request and writes the backend response back to the client
- Uses a single `cleanup:` label with `goto` for centralized resource cleanup
- Tested successfully against `python3 -m http.server 9001`

## Current Code Shape

Everything is in one file (`src/main.c`). The loop is:

```
accept() → read() → connect() → write() → read() → write() → cleanup
```

Cleanup closes `backend_fd` (if it was opened) and `client_fd` in one place.

## Recent Lessons / Bugs Fixed

- `inet_pton()` must write to `&backend_address.sin_addr`, not `sin_family`.
- `connect()` returns `0` on success and `-1` on failure — not a positive/negative polarity.
- `write(client_fd, response, read_response)` must use the actual byte count read, not `sizeof(response)`.
- `goto cleanup` is an idiomatic C pattern for centralized resource cleanup; it is not the same as "goto considered harmful" in higher-level languages.

## Design Decisions in Progress

### Load Balancing: Round-Robin

- Add a `backend_pool` with multiple backends.
- Track `next_index` inside the pool for round-robin selection.
- `connect_to_backend(struct backend *b)` will be a pure helper that creates a socket and attempts `connect()`. It returns the fd or `-1` and does **not** modify backend state.
- `select_backend(struct backend_pool *pool, struct backend **out_backend)` will:
  - Try up to `count` backends.
  - Pick the next backend via round-robin.
  - Skip backends marked dead.
  - Verify liveness by calling `connect_to_backend()`.
  - On success: return the connected fd and write the chosen backend pointer through `out_backend`.
  - On total failure: return `-1`.
- If all backends are dead, the proxy will return a hardcoded `502 Bad Gateway` to the client.

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

- `alive` is reset to `1` on every new request attempt (no background health checks yet).
- `next_index` advances regardless of whether the chosen backend was dead.

### Single-File vs. Multi-File

- For now, code stays in `src/main.c` until round-robin works end-to-end.
- After that, refactor into modules: listener, router/upstream, config, etc.

## Known Limitations

- Only handles one client at a time (blocking loop, no `epoll`/`kqueue` yet).
- Reads the request once with a fixed 4096-byte buffer; large requests are truncated.
- Reads the response once with a fixed 4096-byte buffer; large responses are truncated.
- Does not handle partial writes.
- Does not parse HTTP; just forwards raw bytes.
- Does not support keep-alive or connection reuse.

## Next Steps

1. Implement `connect_to_backend()` helper.
2. Implement `select_backend()` with round-robin and dead-backend skipping.
3. Wire it into the main loop and add the `502 Bad Gateway` path.
4. Test with multiple Python HTTP servers on different ports.
5. Then refactor into multiple source files and start Phase 1 (HTTP/1.1 parsing) or Phase 5 (event-driven core), depending on priorities.
