# Getting Started - Your First Socket Server

> **Note:** This doc was written for the original single-threaded HTTP server tutorial. The project has since pivoted to a **reverse proxy / load balancer**. See [`00-project-overview.md`](00-project-overview.md) and [`06-roadmap.md`](06-roadmap.md) for the current direction. The socket basics below are still valid background.

Ready to write code? Let's build the simplest possible socket server first, then evolve it into an HTTP server.

---

## Step 0: Project Setup

```bash
cd /Users/darlingsuno/workspace/c/projects/http-server
mkdir -p src include
touch src/main.c
touch Makefile
```

---

## Step 1: The Absolute Minimum

**Goal:** Create a socket, bind it, and print "Server started!"

**src/main.c:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main() {
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created: fd=%d\n", server_fd);

    // 2. Allow address reuse (avoid "Address already in use" error)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 3. Setup address structure
    struct sockaddr_in address;
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT);       // Port 8080

    // 4. Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Bound to port %d\n", PORT);

    // 5. Start listening
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    // Keep running
    while (1) {
        sleep(1);
    }

    return 0;
}
```

**Makefile:**
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g

all: server

server: src/main.c
	$(CC) $(CFLAGS) -o server src/main.c

clean:
	rm -f server

test:
	curl http://localhost:8080/

.PHONY: all clean test
```

**Compile and run:**
```bash
make
./server
# Should print:
# Socket created: fd=3
# Bound to port 8080
# Server listening on port 8080...
```

**Test it:**
```bash
# In another terminal:
telnet localhost 8080
# You should connect! (but nothing happens yet)
```

---

## Step 2: Accept One Connection

**Modify main.c** - replace the `while(1)` loop:

```c
// ... (previous code)

printf("Server listening on port %d...\n", PORT);

// NEW: Accept one connection
int client_fd = accept(server_fd, NULL, NULL);
if (client_fd < 0) {
    perror("accept failed");
    exit(EXIT_FAILURE);
}
printf("Client connected! fd=%d\n", client_fd);

// Close connection
close(client_fd);
printf("Connection closed\n");

close(server_fd);
return 0;
```

**Compile and run:**
```bash
make clean && make
./server
```

**Test:**
```bash
telnet localhost 8080
# Server should print "Client connected!" and then close
```

---

## Step 3: Read Data from Client

```c
// Accept connection
int client_fd = accept(server_fd, NULL, NULL);
if (client_fd < 0) {
    perror("accept failed");
    exit(EXIT_FAILURE);
}
printf("Client connected! fd=%d\n", client_fd);

// NEW: Read data
char buffer[4096] = {0};
int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
if (bytes_read < 0) {
    perror("read failed");
} else {
    buffer[bytes_read] = '\0';  // Null terminate
    printf("Received %d bytes:\n%s\n", bytes_read, buffer);
}

close(client_fd);
```

**Test:**
```bash
./server

# In another terminal:
curl http://localhost:8080/hello
```

**You should see the raw HTTP request!**
```
Received 78 bytes:
GET /hello HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.0
Accept: */*
```

---

## Step 4: Send a Response

```c
// Read request (same as before)
char buffer[4096] = {0};
read(client_fd, buffer, sizeof(buffer) - 1);

// NEW: Send HTTP response
char response[] = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";

write(client_fd, response, strlen(response));
printf("Response sent\n");

close(client_fd);
```

**Test:**
```bash
./server

# In another terminal:
curl http://localhost:8080/
# Should print: Hello, World!
```

**🎉 Congratulations! You just built your first HTTP server!**

---

## Step 5: Loop to Handle Multiple Requests

Wrap the accept logic in a loop:

```c
printf("Server listening on port %d...\n", PORT);

while (1) {
    printf("\nWaiting for connection...\n");
    
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept failed");
        continue;
    }
    printf("Client connected! fd=%d\n", client_fd);

    // Read request
    char buffer[4096] = {0};
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Request: %s\n", buffer);
    }

    // Send response
    char response[] = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";
    write(client_fd, response, strlen(response));

    close(client_fd);
    printf("Connection closed\n");
}
```

**Now your server can handle multiple requests!**

```bash
curl http://localhost:8080/
curl http://localhost:8080/anything
curl http://localhost:8080/foo/bar
# All work! (but return the same response)
```

---

## Step 6: Parse the Request

Extract method and path:

```c
// After reading buffer
if (bytes_read > 0) {
    buffer[bytes_read] = '\0';
    
    // Parse request line
    char method[16], path[256], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);
    
    printf("Method: %s, Path: %s, Version: %s\n", method, path, version);
}
```

**Test:**
```bash
curl http://localhost:8080/hello
# Server prints: Method: GET, Path: /hello, Version: HTTP/1.1
```

---

## Step 7: Route Based on Path

```c
// After parsing
char response[1024];

if (strcmp(path, "/") == 0) {
    sprintf(response,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "Welcome to server!");
        
} else if (strcmp(path, "/hello") == 0) {
    sprintf(response,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!");
        
} else {
    sprintf(response,
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 9\r\n"
        "\r\n"
        "Not Found");
}

write(client_fd, response, strlen(response));
```

**Test all routes:**
```bash
curl http://localhost:8080/           # Welcome to server!
curl http://localhost:8080/hello      # Hello, World!
curl http://localhost:8080/notfound   # Not Found
```

---

## What You've Built

In ~150 lines of C, you have:
- ✅ A working TCP socket server
- ✅ HTTP request parsing
- ✅ Routing based on path
- ✅ Proper HTTP responses with status codes
- ✅ Multiple request handling

---

## Next Steps

Now that you have a working prototype, you can:

1. **Refactor into modules**
   - Move parsing to `parser.c`
   - Move routing to `router.c`
   - Create proper data structures

2. **Add features**
   - `/time` route that returns current time
   - `/echo?msg=hello` that echoes query params
   - Better error handling

3. **Add threading** (Phase 2)
   - Handle requests concurrently
   - Learn about pthread

---

## Full Code (All Steps Combined)

Here's the complete working version:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Main loop
    while (1) {
        printf("\nWaiting for connection...\n");
        
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Read request
        char buffer[BUFFER_SIZE] = {0};
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0';

        // Parse
        char method[16], path[256], version[16];
        sscanf(buffer, "%s %s %s", method, path, version);
        printf("Request: %s %s\n", method, path);

        // Route and respond
        char response[1024];
        
        if (strcmp(path, "/") == 0) {
            sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 18\r\n"
                "\r\n"
                "Welcome to server!");
        } else if (strcmp(path, "/hello") == 0) {
            sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "Hello, World!");
        } else {
            sprintf(response,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 9\r\n"
                "\r\n"
                "Not Found");
        }

        write(client_fd, response, strlen(response));
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
```

Save this, compile with `make`, and you have a working HTTP server!

---

**You're ready to start building!** 🚀

Start with this simple version, make sure you understand every line, then gradually add complexity. Good luck!
