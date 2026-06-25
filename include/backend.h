#ifndef BACKEND_H
#define BACKEND_H

struct backend {
  char* ip;
  int port;
  int alive;
};

struct backend_pool {
  struct backend* backends;
  int count;
  int next_index;
};

int connect_backend(struct backend* opt);
int select_backend(struct backend_pool* backend_list, struct backend** chosen_backend);

#endif
