#include "../include/proxy.h"
#include <string.h>
#include <unistd.h>

void proxy_send_bad_gateway(int client_fd) {
  const char *bad_gateway = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: "
                            "0\r\nConnection: close\r\n\r\n";
  write(client_fd, bad_gateway, strlen(bad_gateway));
};

int proxy_send_request(int backend_fd, const char *buffer, size_t req_size) {
  return write(backend_fd, buffer, req_size);
};

int proxy_stream_response(int backend_fd, int client_fd) {
  char response[4096];
  int bytes_read;
  while ((bytes_read = read(backend_fd, response, sizeof(response))) > 0) {
    if (write(client_fd, response, bytes_read) == -1) {
      return -1;
    }
  }
  return bytes_read;
};
