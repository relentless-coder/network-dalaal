#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"
#include "proxy.h"
#include "backend.h"

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

void proxy_send_bad_request(int client_fd) {
  const char *bad_request =
      "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close";
  write(client_fd, bad_request, strlen(bad_request));
}

int proxy_select_backend_and_forward_request(config_t* cfg, int* client_fd, int* backend_fd, buf_t* req_buf, size_t bytes_consumed) {
    struct backend *chosen_backend;
    *backend_fd = select_backend(&cfg->backends, &chosen_backend);
    if (*backend_fd == -1) {
      perror("getting valid backend");
      proxy_send_bad_gateway(*client_fd);
      return -1;
    }
    printf("chose backend port %d\n", chosen_backend->port);
    int file_write = proxy_send_request(*backend_fd, req_buf->data, bytes_consumed);
    if (file_write == -1) {
      perror("error writing the repsonse");
      return -1;
    }
    int bytes_read = proxy_stream_response(*backend_fd, *client_fd);
    if (bytes_read == -1) {
      perror("error reading response from backend");
      return -1;
    }
  return 0;
}
