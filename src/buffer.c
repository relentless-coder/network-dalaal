#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "buffer.h"

int buf_init(buf_t* buf, size_t cap) {
  if (buf == NULL) {
    fprintf(stderr, "ERROR: buf_init: buffer is NULL\n");
    return -1;
  }
  if (cap == 0) {
    fprintf(stderr, "Invalid value for buffer capacity\n");
    return -1;
  }
  char* data = malloc(cap);
  if (data == NULL) {
    return -1;
  }
  buf->data = data;
  buf->len = 0;
  buf->cap = cap;
  return 0;
}

void buf_free(buf_t* buf) {
  if (buf == NULL) {
    fprintf(stderr, "ERROR: buf_free: buffer is NULL\n");
    return;
  }
  free(buf->data);
  buf->cap = 0;
  buf->len = 0;
  buf->data = NULL;
}

int buf_append(buf_t* buf, const char* data, size_t n) {
  if (buf == NULL || data == NULL) {
    return -1;
  }
  char* src = buf_reserve(buf, n);
  if (src == NULL) {
    return -1;
  }
  memcpy(src, data, n);
  buf->len += n;
  return 0;
}

char* buf_reserve(buf_t* buf, size_t n) {
  if (buf->cap - buf->len >= n) {
    return buf->data + buf->len;
  }
  size_t cap = buf->cap;
  if (n > SIZE_MAX - buf->len) {
    errno = ENOMEM;
    return NULL;
  }
  while (cap - buf->len <= n) {
    if (cap > SIZE_MAX/2) {
      errno = ENOMEM;
      return NULL;
    }
    cap = 2*cap;
  }
  char* new_size = realloc(buf->data, cap);
  if (new_size == NULL) {
    return NULL;
  }
  buf->data = new_size;
  buf->cap = cap;
  return buf->data + buf->len;
}
