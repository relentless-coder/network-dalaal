#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

int str_nocase_cmp(const char *a, const char *b, size_t len);
int str_extract_int(const char *data, size_t value_start, size_t value_end);

#endif
