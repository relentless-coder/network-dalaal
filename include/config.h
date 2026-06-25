#ifndef CONFIG_H
#define CONFIG_H

#include "backend.h"

typedef struct {
  char* listen_host;
  int listen_port;
  struct backend_pool backends;
} config_t;

int config_load(const char *path, config_t* cfg);
void config_free(config_t *cfg);

#endif
