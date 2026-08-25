#ifndef PROXY_H
#define PROXY_H

#include <stddef.h>
#include "buffer.h"
#include "config.h"

int proxy_send_request(int backend_fd, const char* request, size_t len);
int proxy_stream_response(int backend_fd, int client_fd);
void proxy_send_bad_gateway(int client_fd);
void proxy_send_bad_request(int client_fd);
int proxy_select_backend_and_forward_request(config_t* cfg, int* client_fd, int* backend_fd, buf_t* req_buf, size_t bytes_consumed);

#endif

