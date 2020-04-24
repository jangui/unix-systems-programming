struct clientList;

struct clientHandlerArgs {
  int *fd;
  struct msgQ *queue;
  pthread_mutex_t *connlock;
  struct clientList *clients;
};

struct queueHandlerArgs {
  struct msgQ *queue;
  pthread_mutex_t *connlock;
  struct clientList *clients;
};

void *client_handler(void *args);
void *queue_handler(void *args);
