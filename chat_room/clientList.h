
struct client {
  int fd;     // client's fd for socket
  char *name; // client's name
};

struct clientList {
  int s;        // size of clientList
  int maxSize;  // max size of clientList
  struct client **lst; // list of client structs
};

struct clientList *initClientList(int maxSize);

int clientAppend(struct clientList *clients, int fd, char *name);
int clientRemove(struct clientList *clients, char *name);
int lookupClient(struct clientList *clients, char *name);
char **getClients(struct clientList *clients, int *c);
