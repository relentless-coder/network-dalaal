#include "buffer.h"

typedef enum {
  METHOD_GET,
  METHOD_POST,
  METHOD_OPTIONS,
  METHOD_PUT,
  METHOD_PATCH,
  METHOD_DELETE,
  METHOD_UNKNOWN
} http_method_t;

typedef enum {
  HTTP_1_0,
  HTTP_1_1,
  HTTP_VERSION_UNKNOWN
} http_version_t;


typedef struct {
  http_method_t method;
  http_version_t version;
  char* path;
  char* body;
  size_t body_len;
  size_t content_length;
  int keep_alive;
  buf_t* raw;
} http_request_t;

int http_parse_request(buf_t* buf, http_request_t* http_req);
