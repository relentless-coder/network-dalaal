#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include "backend.h"
#include "proxy.h"
#include "config.h"


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
    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd == -1) {
      perror("error accepting request on socket");
      continue;
    }
    char buffer[4096];
    int file_read = read(client_fd, buffer, 4096);
    if (file_read == -1) {
      perror("error reading the request");
      goto cleanup;
    }
    struct backend* chosen_backend;
    int backend_fd = select_backend(&cfg.backends, &chosen_backend);
    if (backend_fd == -1) {
      perror("getting valid backend");
      const char* bad_gateway = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      write(client_fd, bad_gateway, strlen(bad_gateway));
      goto cleanup;
    }
    printf("chose backend port %d\n", chosen_backend->port);
    int file_write = write(backend_fd, buffer, file_read);
    if (file_write == -1) {
      perror("error writing the repsonse");
      goto cleanup;
    }
    char response[4096];
    int bytes_read;
    while ((bytes_read = read(backend_fd, response, sizeof(response))) > 0) {
      if (write(client_fd, response, bytes_read) == -1) {
        perror("error writing to clinet");
        goto cleanup;
      }
    }
    if (bytes_read == -1) {
      perror("error reading response from backend");
      goto cleanup;
    }
  cleanup:
    printf("closing backend and clinet fd %d,%d\n", backend_fd, client_fd);
    if (backend_fd != -1) {
      close(backend_fd);
    }
    close(client_fd);
  }
}
