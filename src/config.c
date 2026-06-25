#include "config.h"
#include "cJSON.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int config_load(const char *path, config_t *cfg) {
  int res = -1;
  char *f_buff = NULL;
  cJSON *root = NULL;
  config_t tmp;
  FILE *f = NULL;
  memset(&tmp, 0, sizeof(tmp));
  f = fopen(path, "rb");
  if (f == NULL) {
    perror("fopen: ");
    goto cleanup;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  if (size == -1) {
    perror("ftell:");
    goto cleanup;
  }
  if (size == 0) {
    goto cleanup;
  }
  rewind(f);

  f_buff = malloc(size + 1);
  if (f_buff == NULL) {
    perror("error allocating memory");
    goto cleanup;
  }

  size_t bytes_read = fread(f_buff, 1, size, f);
  if (bytes_read < (size_t)size) {
    if (ferror(f)) {
      fprintf(stderr, "error reading file\n");
      goto cleanup;
    } else if (feof(f)) {
      fprintf(stderr, "error reached end of file unexpectedly\n");
      goto cleanup;
    }
  }

  f_buff[bytes_read] = '\0';

  root = cJSON_Parse(f_buff);

  if (root == NULL) {
    fprintf(stderr, "Error parsing the error\n");
    goto cleanup;
  }

  const cJSON *host = cJSON_GetObjectItem(root, "listen_host");
  const cJSON *port = cJSON_GetObjectItem(root, "listen_port");
  const cJSON *backends = cJSON_GetObjectItem(root, "backends");
  if (!cJSON_IsString(host)) {
    fprintf(stderr, "Invalid ip type");
    goto cleanup;
  }
  if (!cJSON_IsNumber(port)) {
    fprintf(stderr, "Invalid port type");
    goto cleanup;
  }

  tmp.listen_host = strdup(host->valuestring);
  tmp.listen_port = port->valueint;
  if (!backends || cJSON_IsArray(backends) == 0) {
    fprintf(stderr, "Missing backends");
    goto cleanup;
  }
  int items = cJSON_GetArraySize(backends);
  tmp.backends.backends = calloc(items, sizeof(struct backend));
  if (tmp.backends.backends == NULL) {
    fprintf(stderr, "Error allocating memory for backends");
    goto cleanup;
  }
  for (int i = 0; i < items; i++) {
    cJSON *backend_item = cJSON_GetArrayItem(backends, i);
    cJSON *ip_obj = cJSON_GetObjectItem(backend_item, "ip");
    cJSON *port_obj = cJSON_GetObjectItem(backend_item, "port");
    if (ip_obj == NULL || port_obj == NULL) {
      fprintf(stderr, "Invalid backend config");
      goto cleanup;
    }
    tmp.backends.backends[i].ip = strdup(ip_obj->valuestring);
    tmp.backends.backends[i].port = port_obj->valueint;
    tmp.backends.count = tmp.backends.count + 1;
  }
  tmp.backends.next_index = 0;
  res = 0;

cleanup:
  if (f != NULL) {
    fclose(f);
  }
  if (f_buff != NULL) {
    free(f_buff);
  }
  if (root != NULL) {
    cJSON_Delete(root);
  }
  if (res == 0) {
    *cfg = tmp;
  } else {
    config_free(&tmp);
  }

  return res;
}

void config_free(config_t *cfg) {
  if (cfg == NULL) {
    return;
  }
  free(cfg->listen_host);
  for (int i = 0; i < cfg->backends.count; i++) {
    free(cfg->backends.backends[i].ip);
  }
  free(cfg->backends.backends);
  memset(cfg, 0, sizeof(*cfg));
}
