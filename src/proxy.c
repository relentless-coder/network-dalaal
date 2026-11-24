#include "proxy.h"
#include "backend.h"
#include "config.h"
#include "http.h"
#include <errno.h>
#include <stdio.h>
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
  buf_t res_buf = {0};
  int buf_ready = buf_init(&res_buf, 4096);
  if (buf_ready == -1) {
    fprintf(stderr, "Error: initialising response buffer\n");
    return PARSE_ERROR;
  }
  http_response_t http_res = {0};
  size_t bytes_consumed = 0;
  while (1) {
    fprintf(stdout, "Reading response\n");
    char *write_pos = buf_reserve(&res_buf, 4096);
    bytes_read = read(backend_fd, write_pos, res_buf.cap - res_buf.len);
    if (bytes_read == -1) {
      fprintf(stderr, "error reading the response\n");
      return PARSE_ERROR;
    }
    if (bytes_read == 0) {
      fprintf(stdout, "Client closed connection\n");
      return PARSE_ERROR;
    }
    res_buf.len += bytes_read;
    parse_status_t parse_status =
        http_parse_response(&res_buf, &http_res, &bytes_consumed);
    if (parse_status == PARSE_OK) {
      if (write(client_fd, http_res.raw, bytes_consumed) == -1) {
        return -1;
      }
      return parse_status;
    } else {
      return parse_status;
    }
  }
  return bytes_read;
};

void proxy_send_bad_request(int client_fd) {
  const char *bad_request =
      "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close";
  write(client_fd, bad_request, strlen(bad_request));
}

int proxy_select_backend_and_forward_request(config_t *cfg, int *client_fd,
                                             int *backend_fd, buf_t *req_buf,
                                             size_t bytes_consumed) {
  struct backend *chosen_backend;
  *backend_fd = select_backend(&cfg->backends, &chosen_backend);
  if (*backend_fd == -1) {
    perror("getting valid backend");
    proxy_send_bad_gateway(*client_fd);
    return -1;
  }
  printf("chose backend port %d\n", chosen_backend->port);
  int file_write =
      proxy_send_request(*backend_fd, req_buf->data, bytes_consumed);
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
