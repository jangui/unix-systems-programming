struct clientList;

struct clientHandlerArgs {
  int fd;
  pthread_mutex_t *connlock;
  struct clientList *clients;
};

void *client_handler(void *(args));
