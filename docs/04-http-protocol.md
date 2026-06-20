# HTTP Protocol Basics

## The Key Insight

**HTTP is just formatted text sent over TCP sockets.**

When you type `curl http://localhost:8080/hello`, the `curl` program opens a TCP socket and sends plain text. Your server reads that text, parses it, and sends text back.

---

## What an HTTP Request Looks Like

When a client connects, this is what gets sent:

```
GET /hello HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: curl/7.0\r\n
Accept: */*\r\n
\r\n
```

**That's it!** Just text with `\r\n` (carriage return + newline) separating lines.

---

## Breaking Down the Request

### Line 1: Request Line
```
GET /hello HTTP/1.1\r\n
```

**Format:** `METHOD PATH VERSION\r\n`

- **Method**: `GET`, `POST`, `PUT`, `DELETE`, etc.
- **Path**: `/hello` (what they're requesting)
- **Version**: `HTTP/1.1` or `HTTP/1.0`

### Lines 2-N: Headers
```
Host: localhost:8080\r\n
User-Agent: curl/7.0\r\n
Accept: */*\r\n
```

**Format:** `Header-Name: value\r\n`

Common headers:
- `Host`: Which domain (required in HTTP/1.1)
- `User-Agent`: What client software
- `Accept`: What content types the client accepts
- `Content-Length`: Size of request body (for POST)
- `Content-Type`: Format of request body

### Blank Line: End of Headers
```
\r\n
```

This signals "headers are done, body starts next" (if any).

### Optional: Request Body

For `POST`/`PUT` requests:
```
POST /api/data HTTP/1.1\r\n
Host: localhost:8080\r\n
Content-Type: application/json\r\n
Content-Length: 27\r\n
\r\n
{"name": "Alice", "age": 30}
```

---

## What an HTTP Response Looks Like

Your server sends back:

```
HTTP/1.1 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 13\r\n
\r\n
Hello, World!
```

---

## Breaking Down the Response

### Line 1: Status Line
```
HTTP/1.1 200 OK\r\n
```

**Format:** `VERSION STATUS_CODE STATUS_TEXT\r\n`

Common status codes:
- `200 OK` - Success
- `404 Not Found` - Resource doesn't exist
- `500 Internal Server Error` - Server crashed
- `301 Moved Permanently` - Redirect
- `400 Bad Request` - Client sent invalid request

### Lines 2-N: Response Headers
```
Content-Type: text/plain\r\n
Content-Length: 13\r\n
```

Common response headers:
- `Content-Type`: What format the body is (text/html, application/json, etc.)
- `Content-Length`: Size of response body in bytes
- `Connection: close` or `Connection: keep-alive`
- `Date`: When response was generated

### Blank Line
```
\r\n
```

### Response Body
```
Hello, World!
```

The actual content (HTML, JSON, file data, etc.)

---

## Minimal Valid Response

**Simplest possible response:**
```
HTTP/1.1 200 OK\r\n
\r\n
```

No headers, no body! Browsers will accept it (though you *should* include `Content-Length`).

---

## Reading HTTP in C

### Step 1: Read from Socket

```c
char buffer[4096];
int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
buffer[bytes_read] = '\0';  // Null-terminate

// buffer now contains:
// "GET /hello HTTP/1.1\r\nHost: localhost:8080\r\n...\r\n\r\n"
```

### Step 2: Parse Request Line

```c
char method[16], path[256], version[16];
sscanf(buffer, "%s %s %s", method, path, version);

// method = "GET"
// path = "/hello"
// version = "HTTP/1.1"
```

### Step 3: (Optional) Parse Headers

```c
char *line = strtok(buffer, "\r\n");  // Skip request line
while ((line = strtok(NULL, "\r\n")) != NULL) {
    if (strlen(line) == 0) break;  // Empty line = end of headers
    
    // Parse "Header-Name: value"
    char *colon = strchr(line, ':');
    if (colon) {
        *colon = '\0';
        char *header_name = line;
        char *header_value = colon + 2;  // Skip ": "
        // Store or process header
    }
}
```

---

## Sending HTTP Response in C

### Simple Example

```c
char response[1024];
sprintf(response,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!");

write(client_fd, response, strlen(response));
```

### With Dynamic Content

```c
char body[] = "Hello, World!";
int body_length = strlen(body);

char response[1024];
int header_length = sprintf(response,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    body_length);

// Send headers
write(client_fd, response, header_length);

// Send body
write(client_fd, body, body_length);
```

---

## HTTP Request Methods

| Method | Purpose | Has Body? |
|--------|---------|-----------|
| GET    | Retrieve resource | No |
| POST   | Create resource | Yes |
| PUT    | Update resource | Yes |
| DELETE | Delete resource | No |
| HEAD   | Like GET but no body | No |
| OPTIONS| What methods allowed | No |

For your first version, just implement **GET**.

---

## Important Details

### Content-Length is Critical

```c
// ❌ Wrong - browser doesn't know where body ends
"HTTP/1.1 200 OK\r\n\r\nHello"

// ✅ Right
"HTTP/1.1 200 OK\r\n"
"Content-Length: 5\r\n"
"\r\n"
"Hello"
```

### Line Endings Matter

**Must be `\r\n` (CRLF), not just `\n`!**

```c
// ❌ Wrong
"HTTP/1.1 200 OK\n\n"

// ✅ Right
"HTTP/1.1 200 OK\r\n\r\n"
```

### How to Know Request is Complete?

Look for `\r\n\r\n` (double CRLF) - signals end of headers.

```c
if (strstr(buffer, "\r\n\r\n") != NULL) {
    // Request is complete
}
```

For requests with bodies, you need to read `Content-Length` bytes after the headers.

---

## Query Parameters

Request: `GET /echo?msg=hello&name=alice HTTP/1.1`

The path is `/echo?msg=hello&name=alice`

Parse it:
```c
char *query = strchr(path, '?');
if (query) {
    *query = '\0';  // path is now "/echo"
    query++;        // query is now "msg=hello&name=alice"
    
    // Parse key=value pairs split by &
}
```

---

## Common Pitfalls

1. **Forgetting `\r\n`** - Use CRLF, not just LF
2. **Wrong Content-Length** - Count bytes, not characters (matters for UTF-8)
3. **Not null-terminating** - After `read()`, add `\0` to use string functions
4. **Assuming one `read()` gets entire request** - Might need multiple reads
5. **Not closing connection** - Always `close(client_fd)` when done

---

## Testing Your Server

### Using curl:
```bash
curl -v http://localhost:8080/hello
```

The `-v` flag shows the full HTTP exchange.

### Using telnet:
```bash
telnet localhost 8080
GET /hello HTTP/1.1
Host: localhost
[press Enter twice]
```

### Using browser:
Just visit `http://localhost:8080/hello`

---

## Next Steps

Your HTTP server needs to:

1. **Accept connection** → get client socket
2. **Read request** → until you see `\r\n\r\n`
3. **Parse request line** → extract method and path
4. **Route** → match path to handler
5. **Build response** → format with proper headers
6. **Send response** → write to client socket
7. **Close connection** → clean up

Start simple - just return "Hello" for all requests. Then add routing, then query params, etc.
