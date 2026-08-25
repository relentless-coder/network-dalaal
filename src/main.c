#include "buffer.h"
#include "config.h"
#include "http.h"
#include "proxy.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("error creating socket connection");
    return EXIT_FAILURE;
  }
  int opt = 1;
  config_t cfg = {0};
  if (config_load("config.json", &cfg) != 0) {
    fprintf(stderr, "error loading the config");
    return EXIT_FAILURE;
  }
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(cfg.listen_host);
  address.sin_port = htons(cfg.listen_port);
  int bnd = bind(socket_fd, (struct sockaddr *)&address, sizeof(address));
  if (bnd == -1) {
    perror("error binding socket to address");
    return EXIT_FAILURE;
  }
  int lstn = listen(socket_fd, SOMAXCONN);
  if (lstn == -1) {
    perror("error listening on socket");
    return EXIT_FAILURE;
  }

  while (1) {
    int backend_fd = -1;
    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd == -1) {
      perror("error accepting request on socket");
      continue;
    }

    buf_t req_buf = {0};
    int buf_ready = buf_init(&req_buf, 4096);
    if (buf_ready == -1) {
      fprintf(stderr, "Error: initialising request buffer");
      goto cleanup;
    }
    http_request_t http_req = {0};
    size_t bytes_consumed = 0;
    while (1) {
      fprintf(stdout, "Reading request\n");
      char *write_pos = buf_reserve(&req_buf, 4096);
      fprintf(stdout, "buf reserve %zu\n", req_buf.cap - req_buf.len);
      int file_read = read(client_fd, write_pos, req_buf.cap - req_buf.len);
      if (file_read == -1) {
        fprintf(stderr, "error reading the request\n");
        perror("error reading the request\n");
        goto cleanup;
      }
      fprintf(stdout, "no error reading the request\n");
      if (file_read == 0) {
        fprintf(stdout, "Client closed connection\n");
        goto cleanup;
      }
      req_buf.len += file_read;
      parse_status_t parse_status =
          http_parse_request(&req_buf, &http_req, &bytes_consumed);
      fprintf(stdout, "parse status %d\n", parse_status);
      if (parse_status == PARSE_OK) {
        int request_forwarded = proxy_select_backend_and_forward_request(
            &cfg, &client_fd, &backend_fd, &req_buf, bytes_consumed);
        if (request_forwarded == -1)
          goto cleanup;
        if (http_req.keep_alive == 0)
          break;
        buf_reset(&req_buf, 4096);
        memset(&http_req, 0, sizeof(http_req));
        bytes_consumed = 0;
      }
      if (parse_status == PARSE_ERROR) {
        perror("bad request");
        proxy_send_bad_request(client_fd);
        goto cleanup;
      }
      if (parse_status == PARSE_NEED_DATA) {
        fprintf(stdout, "Need more request data");
        goto cleanup;
      }
    }
  cleanup:
    printf("closing backend and clinet fd %d,%d\n", backend_fd, client_fd);
    if (backend_fd != -1) {
      close(backend_fd);
    }
    if (http_req.keep_alive == 0) {
      buf_free(&req_buf);
      close(client_fd);
    } else {
      buf_free(&req_buf);
      memset(&http_req, 0, sizeof(http_req));
    }
  }
}
