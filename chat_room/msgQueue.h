struct msgNode;

struct msgNode {
  char *msg;
  struct msgNode *next;
  struct msgNode *prev;
};

struct msgQ {
  int s;
  int maxSize;
  struct msgNode *head;
  struct msgNode *tail;
  pthread_mutex_t *quetex;
  pthread_cond_t *condc;
  pthread_cond_t *condp;
};

struct msgQ *initQ(int maxSize);
int enqueue(struct msgQ *queue, char *msg);
int skipQueue(struct msgQ *queue, char *msg);
char *dequeue(struct msgQ *queue);
void destroyQ(struct msgQ *queue);
