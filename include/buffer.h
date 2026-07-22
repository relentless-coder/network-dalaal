#include <stddef.h>

typedef struct {
  char* data;
  size_t len;
  size_t cap;
} buf_t;

int buf_init(buf_t* buf, size_t cap);
void buf_free(buf_t* buf);
int buf_append(buf_t* buf,const char* data, size_t n);
char* buf_reserve(buf_t* buf, size_t n);
