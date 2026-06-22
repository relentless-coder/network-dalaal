#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

struct backend {
  const char *ip;
  int port;
  int alive;
};

struct backend_pool {
  struct backend *backends;
  int count;
  int next_index;
};

int connect_backend(struct backend *option) {
  int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (backend_fd == -1) {
    perror("error creating socket for backend");
    return -1;
  }
  struct sockaddr_in backend_address;
  backend_address.sin_family = AF_INET;
  backend_address.sin_port = htons(option->port);
  if (inet_pton(AF_INET, option->ip, &backend_address.sin_addr) <= 0) {
    perror("error setting the hostname for backend");
    close(backend_fd);
    return -1;
  }
  if (connect(backend_fd, (struct sockaddr *)&backend_address,
              sizeof(backend_address)) == -1) {
    perror("error connecting to backend server");
    close(backend_fd);
    return -1;
  }
  return backend_fd;
};

int select_backend(struct backend_pool *backend_list,
                   struct backend **chosen_backend) {
  int loop = 0;
  int backend_fd = -1;
  while (loop < backend_list->count) {
    int i = backend_list->next_index;
    struct backend *target_backend = backend_list->backends + i;
    printf("target backend ip %s and port %d\n", target_backend->ip, target_backend->port);
    backend_fd = connect_backend(target_backend);
    if (backend_list->next_index + 1 == backend_list->count) {
      backend_list->next_index = 0;
    } else {
      backend_list->next_index += 1;
    }
    if (backend_fd != -1) {
      *chosen_backend = target_backend;
      target_backend->alive = 1;
      break;
    }
    target_backend->alive = 0;
    loop += 1;
  }

  return backend_fd;
}

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("error creating socket connection");
    return EXIT_FAILURE;
  }
  struct backend backends[] = {
    {"127.0.0.1", 9001, 1},
    {"127.0.0.1", 9002, 1},
    {"127.0.0.1", 9003, 1}
  };
  struct backend_pool backend_list = {
    .backends = backends,
    .count = 3,
    .next_index = 0
  };
  int opt = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);
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
    int backend_fd = select_backend(&backend_list, &chosen_backend);
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
