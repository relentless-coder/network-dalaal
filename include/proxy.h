#ifndef PROXY_H
#define PROXY_H

#include <stddef.h>

int proxy_send_request(int backend_fd, const char* request, size_t len);
int proxy_stream_response(int backend_fd, int client_fd);
void proxy_send_bad_gateway(int client_fd);

#endif

