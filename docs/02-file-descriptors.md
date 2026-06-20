# File Descriptors

## What is a File Descriptor?

In Unix/Linux, **everything is a file**. The OS treats many things like files:

- Actual files on disk
- Your keyboard (stdin)
- Your screen (stdout)
- Network connections (sockets)
- Pipes between processes

**A file descriptor (fd) is just a number** that represents an open "file" (in the broad sense).

---

## The "Coat Check" Analogy

Think of it like a coat check:
- You drop off your coat (open a socket/file)
- They give you a numbered ticket: "Here's #5"
- That number (#5) lets you get your coat back later
- **The number itself isn't the coat** — it's a reference to it

---

## Reserved File Descriptors

Every program starts with three open file descriptors:

```c
0 = stdin  (keyboard input)
1 = stdout (screen output)
2 = stderr (error output)
```

When you do `printf("hello")`, it's actually `write(1, "hello", 5)` under the hood.

---

## Example: Opening Files

```c
int fd1 = open("myfile.txt", O_RDONLY);  // fd1 = 3
int fd2 = open("other.txt", O_RDONLY);   // fd2 = 4
```

The OS says "I opened that file for you, here's number 3 to refer to it."

---

## Sockets are Just Special File Descriptors

```c
int server_socket = socket(AF_INET, SOCK_STREAM, 0);
// server_socket might be 3 (just a number!)

int client_socket = accept(server_socket, ...);
// client_socket might be 4

// Now you can read/write like a file:
char buffer[1024];
read(client_socket, buffer, sizeof(buffer));   // Receive data
write(client_socket, "Hello", 5);              // Send data
close(client_socket);                          // Done
```

**Key insight:** The OS doesn't care if fd 3 is a file or a socket. You use the same `read()`, `write()`, `close()` functions!

---

## How FD Numbers Get Assigned

The kernel always gives you the **lowest available number** ≥ 3.

### Example Sequence:

```c
int sock1 = socket(...);  // Gets 3 (first available)
int file1 = open("a.txt", ...);  // Gets 4 (next available)
int sock2 = socket(...);  // Gets 5 (next available)

close(sock1);  // 3 is now free again

int sock3 = socket(...);  // Gets 3! (reuses freed slot)
```

**FD Table After:**
```
┌────┬──────────┐
│ 0  │ stdin    │
│ 1  │ stdout   │
│ 2  │ stderr   │
│ 3  │ sock3    │  ← Reused!
│ 4  │ file1    │
│ 5  │ sock2    │
└────┴──────────┘
```

---

## The File Descriptor Table

Every process has a **file descriptor table** maintained by the kernel:

```
Your Process's FD Table:
┌────┬──────────────────────────┐
│ FD │ Points to...             │
├────┼──────────────────────────┤
│ 0  │ → stdin (keyboard)       │
│ 1  │ → stdout (terminal)      │
│ 2  │ → stderr                 │
│ 3  │ → Socket object (kernel) │
│ 4  │ → File object (myfile.txt)│
│ 5  │ → Socket object (client) │
└────┴──────────────────────────┘
```

---

## What Happens When You Create a Socket

```c
int server_sock = socket(AF_INET, SOCK_STREAM, 0);
// Returns 3 (for example)
```

**Behind the scenes:**

1. **Kernel creates a socket object** in kernel memory
2. **Kernel finds next available FD** in your process's table (3)
3. **Kernel creates mapping**: `FD Table[3] → Socket Object`
4. **Returns 3 to your program**

---

## Important: Never Hardcode FD Numbers!

**Wrong:**
```c
socket(...);
read(3, buf, size);  // ❌ Assumes socket is always 3
```

**Right:**
```c
int sock = socket(...);
read(sock, buf, size);  // ✅ Use the variable
```

The actual number doesn't matter — just use the variable the kernel gave you!

---

## Key Takeaways

1. **File descriptor = ticket number** (just an integer)
2. **The actual resource** (file/socket) lives in the kernel
3. **FD numbers are reused** when you close resources
4. **Always use the returned value**, never assume a specific number
5. **Same API** (`read`/`write`/`close`) works for files, sockets, pipes, etc.

---

## Questions to Test Understanding

**Q: If you `open()` three files and `socket()` one connection, what fd numbers might you get?**

A: 3, 4, 5, 6 (assuming stdin/stdout/stderr are 0, 1, 2)

**Q: What happens if you forget to `close()` a file descriptor?**

A: Resource leak! The kernel keeps that resource allocated. If you open too many without closing, you'll hit the per-process limit (usually 1024 or 4096) and new `open()`/`socket()` calls will fail.

**Q: When you call `read(5, ...)`, how does the kernel know what to read?**

A: It looks up FD 5 in your process's table, finds the associated kernel object (file/socket/pipe), and reads from that object's buffer.
