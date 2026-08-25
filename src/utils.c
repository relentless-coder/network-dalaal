#include "utils.h"

int str_nocase_cmp(const char *a, const char *b, size_t len) {
  for (size_t i = 0; i < len; i++) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z')
      ca += 32;
    if (cb >= 'A' && cb <= 'Z')
      cb += 32;
    if (ca != cb)
      return 1;
  }
  return 0;
}

int str_extract_int(const char *data, size_t value_start, size_t value_end) {
  size_t n = 0;
  for (size_t i = value_start; i < value_end; i++) {
    char c = data[i];
    if (c < '0' || c > '9') {
      return -1;
    }
    n = n * 10 + (c - '0');
  }
  return (int)n;
}
