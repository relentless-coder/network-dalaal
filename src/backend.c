#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "backend.h"


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
