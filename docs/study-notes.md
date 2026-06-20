# HTTP Server - Study Notes

## Socket Fundamentals

### Q: What happens when you call `server.listen(8080)` in Node.js?

**A:** The OS creates a socket (kernel data structure with buffers) and maintains a mapping: `port 8080 → socket → your process ID`. When TCP packets arrive with destination port 8080, the kernel checks this table, finds your socket, and puts data in that socket's receive buffer. Your process reads from the file descriptor.

**Key insight:** Ports aren't physical things that "open." They're 16-bit numbers in packet headers. Think of them as mailbox numbers — you tell the OS "I'm claiming mailbox 8080, send all mail for 8080 to me."

---

### Q: Can two programs listen on the same port?

**A:** No. Only one program can bind to a port at a time. If you try, you get: `Error: bind: Address already in use`

(Exceptions exist with `SO_REUSEADDR` and `SO_REUSEPORT`, but that's advanced.)

---

### Q: Why does `accept()` return a NEW file descriptor instead of reusing the listening socket?

**A:** The listening socket is like a hotel receptionist — it waits for new clients. Each accepted connection gets its own dedicated file descriptor so you can communicate with multiple clients simultaneously.

```
Client A connects → accept() returns fd=4 → read/write to Client A via fd=4
Client B connects → accept() returns fd=5 → read/write to Client B via fd=5
```

The listening socket stays open, ready to accept more connections. Only the individual client connections close after you're done.

---

### Q: What parameters does `socket()` need?

**A:** Three parameters:
```c
int socket(int domain, int type, int protocol);
```

1. **`domain`** (Address Family) — What addressing system?
   - `AF_INET` = IPv4 (e.g., `192.168.1.1`)
   - `AF_INET6` = IPv6
   - `AF_UNIX` = local inter-process communication

2. **`type`** — Socket behavior:
   - `SOCK_STREAM` = connection-oriented, reliable, ordered (TCP)
   - `SOCK_DGRAM` = connectionless, unreliable (UDP)

3. **`protocol`** — Specific protocol (usually `0` = "use default for this domain+type")

**For HTTP server:**
```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // IPv4 TCP socket
```

Returns a file descriptor (positive integer) on success, `-1` on failure.

---

### Q: Why TCP for HTTP?

**A:** HTTP requires:
- Reliable delivery (requests/responses can't get lost)
- Ordered data (packets must arrive in sequence)
- Connection-based communication

TCP provides all of these. UDP doesn't guarantee delivery or order.

---
