#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    perror("failed to open config file");
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("failed to seek config file");
    fclose(fp);
    return NULL;
  }

  long size = ftell(fp);
  if (size < 0) {
    perror("failed to tell config file size");
    fclose(fp);
    return NULL;
  }

  rewind(fp);

  char *buf = malloc((size_t)size + 1);
  if (!buf) {
    fprintf(stderr, "failed to allocate config buffer\n");
    fclose(fp);
    return NULL;
  }

  size_t read = fread(buf, 1, (size_t)size, fp);
  if (read != (size_t)size) {
    fprintf(stderr, "failed to read config file\n");
    free(buf);
    fclose(fp);
    return NULL;
  }

  buf[size] = '\0';
  fclose(fp);
  return buf;
}

int config_load(const char *path, config_t *cfg) {
  if (!path || !cfg) {
    fprintf(stderr, "invalid arguments to config_load\n");
    return -1;
  }

  memset(cfg, 0, sizeof(*cfg));

  char *content = read_file(path);
  if (!content) {
    return -1;
  }

  cJSON *root = cJSON_Parse(content);
  free(content);

  if (!root) {
    fprintf(stderr, "failed to parse config JSON\n");
    return -1;
  }

  cJSON *listen_host = cJSON_GetObjectItemCaseSensitive(root, "listen_host");
  cJSON *listen_port = cJSON_GetObjectItemCaseSensitive(root, "listen_port");
  cJSON *backends = cJSON_GetObjectItemCaseSensitive(root, "backends");

  if (!cJSON_IsString(listen_host) || !cJSON_IsNumber(listen_port) ||
      !cJSON_IsArray(backends)) {
    fprintf(stderr,
            "config missing required fields or wrong types: listen_host, "
            "listen_port, backends\n");
    cJSON_Delete(root);
    return -1;
  }

  strncpy(cfg->listen_host, listen_host->valuestring,
          sizeof(cfg->listen_host) - 1);
  cfg->listen_host[sizeof(cfg->listen_host) - 1] = '\0';
  cfg->listen_port = listen_port->valueint;

  int count = cJSON_GetArraySize(backends);
  if (count <= 0) {
    fprintf(stderr, "config must have at least one backend\n");
    cJSON_Delete(root);
    return -1;
  }

  cfg->backends.backends = calloc((size_t)count, sizeof(struct backend));
  if (!cfg->backends.backends) {
    fprintf(stderr, "failed to allocate backends array\n");
    cJSON_Delete(root);
    return -1;
  }
  cfg->backends.count = count;
  cfg->backends.next_index = 0;

  for (int i = 0; i < count; i++) {
    cJSON *item = cJSON_GetArrayItem(backends, i);
    cJSON *ip = cJSON_GetObjectItemCaseSensitive(item, "ip");
    cJSON *port = cJSON_GetObjectItemCaseSensitive(item, "port");

    if (!cJSON_IsString(ip) || !cJSON_IsNumber(port)) {
      fprintf(stderr, "backend %d has invalid ip or port\n", i);
      config_free(cfg);
      cJSON_Delete(root);
      return -1;
    }

    cfg->backends.backends[i].ip = strdup(ip->valuestring);
    cfg->backends.backends[i].port = port->valueint;
    cfg->backends.backends[i].alive = 1;
  }

  cJSON_Delete(root);
  return 0;
}

void config_free(config_t *cfg) {
  if (!cfg) {
    return;
  }

  if (cfg->backends.backends) {
    for (int i = 0; i < cfg->backends.count; i++) {
      free((char *)cfg->backends.backends[i].ip);
    }
    free(cfg->backends.backends);
    cfg->backends.backends = NULL;
  }

  cfg->backends.count = 0;
  cfg->backends.next_index = 0;
}
