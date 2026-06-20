# Sockets Basics

## What is a Socket?

A socket is an **endpoint for network communication**. Think of it like a telephone:

- **Server socket** = A phone number that's published, waiting for calls
- **Client socket** = The phone you use to call that number  
- **Connection** = An active phone call between two sockets

**In code terms:**
- Socket = a file descriptor (just an integer)
- You `read()` from it to receive data
- You `write()` to it to send data

---

## TCP vs UDP

### TCP (what HTTP uses)
- **Reliable**: guarantees delivery and order
- **Connection-based**: establish connection → exchange data → close
- Like a phone call

### UDP
- **Unreliable**: fire and forget
- **No connection**
- Like sending postcards

HTTP uses **TCP** because we need reliable, ordered delivery.

---

## Client-Server Model

### Server Steps:
1. **Create socket** - Get a phone
2. **Bind to port** - Assign a phone number (e.g., port 8080)
3. **Listen** - Turn on "accepting calls" mode
4. **Accept** - Pick up when someone calls → get a NEW socket for this specific connection
5. **Read/Write** - Exchange data on that connection socket
6. **Close** - Hang up

### Client Steps:
1. **Create socket**
2. **Connect** to server's IP:port
3. **Read/Write**
4. **Close**

---

## Ports and Addresses

- **IP address** = Which computer (e.g., `127.0.0.1` = localhost)
- **Port** = Which program on that computer (e.g., `8080`)
- Together: `127.0.0.1:8080` identifies your HTTP server

### Common Ports:
- 80 = HTTP
- 443 = HTTPS
- 8080 = Common dev HTTP port
- 22 = SSH
- 3306 = MySQL

---

## The Network Stack

```
┌─────────────────┐
│   HTTP Layer    │  ← Your code parses this
├─────────────────┤
│   TCP Layer     │  ← Sockets API handles this
├─────────────────┤
│   IP Layer      │  ← OS handles this
└─────────────────┘
```

Your job: Use sockets (TCP) to send/receive HTTP-formatted text.

---

## Key Questions

**Q: Why does a server need both a "listening socket" and "connection sockets"?**

A: The listening socket is like a receptionist - it waits for new connections. When someone connects, `accept()` creates a NEW socket just for that conversation. The listening socket keeps listening for more connections while you talk on the connection sockets.

**Q: If you run your server on port 8080, what URL would you visit?**

A: `http://localhost:8080/` or `http://127.0.0.1:8080/`

**Q: What happens if you try to bind to port 80 without root permissions?**

A: Error! Ports below 1024 are "privileged" and require root/sudo. That's why dev servers use 8080, 3000, etc.
