#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include "backend.h"
#include "config.h"
#include "proxy.h"

static volatile sig_atomic_t running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static int setup_signals(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; /* do not set SA_RESTART so accept() returns EINTR */

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    return -1;
  }
  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <config.json>\n", argv[0]);
    return EXIT_FAILURE;
  }

  config_t cfg;
  if (config_load(argv[1], &cfg) == -1) {
    return EXIT_FAILURE;
  }

  if (setup_signals() == -1) {
    perror("failed to set up signal handlers");
    config_free(&cfg);
    return EXIT_FAILURE;
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("error creating socket connection");
    config_free(&cfg);
    return EXIT_FAILURE;
  }

  int opt = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(cfg.listen_port);

  if (strcmp(cfg.listen_host, "0.0.0.0") == 0) {
    address.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (inet_pton(AF_INET, cfg.listen_host, &address.sin_addr) <= 0) {
      fprintf(stderr, "invalid listen_host: %s\n", cfg.listen_host);
      close(socket_fd);
      config_free(&cfg);
      return EXIT_FAILURE;
    }
  }

  int bnd = bind(socket_fd, (struct sockaddr *)&address, sizeof(address));
  if (bnd == -1) {
    perror("error binding socket to address");
    close(socket_fd);
    config_free(&cfg);
    return EXIT_FAILURE;
  }

  int lstn = listen(socket_fd, SOMAXCONN);
  if (lstn == -1) {
    perror("error listening on socket");
    close(socket_fd);
    config_free(&cfg);
    return EXIT_FAILURE;
  }

  printf("proxy listening on %s:%d\n", cfg.listen_host, cfg.listen_port);

  while (running) {
    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd == -1) {
      if (errno == EINTR && !running) {
        break;
      }
      perror("error accepting request on socket");
      continue;
    }

    char buffer[4096];
    int file_read = read(client_fd, buffer, 4096);
    if (file_read == -1) {
      perror("error reading the request");
      goto cleanup;
    }

    struct backend *chosen_backend;
    int backend_fd = select_backend(&cfg.backends, &chosen_backend);
    if (backend_fd == -1) {
      perror("getting valid backend");
      proxy_send_bad_gateway(client_fd);
      goto cleanup;
    }

    printf("chose backend port %d\n", chosen_backend->port);
    int file_write = proxy_send_request(backend_fd, buffer, file_read);
    if (file_write == -1) {
      perror("error writing the repsonse");
      goto cleanup;
    }

    int bytes_read = proxy_stream_response(backend_fd, client_fd);
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

  printf("shutting down gracefully\n");
  close(socket_fd);
  config_free(&cfg);
  return EXIT_SUCCESS;
}
