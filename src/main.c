#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("error creating socket connection");
    return EXIT_FAILURE;
  }
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
    char buffer[1024];
    int file_read = read(client_fd, buffer, 1024);
    if (file_read == -1) {
      perror("error reading the request");
    } else {
      char response[20] = "HTTP/1.1 200 OK\r\n\r\n";
      int file_write = write(client_fd, response, 19);
      if (file_write == -1) {
        perror("error writing the repsonse");
      }
    }
    close(client_fd);
  }
}
