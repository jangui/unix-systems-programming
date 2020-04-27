#include <pthread.h> //threads & mutexes
#include <stdlib.h> //malloc, exit
#include <stdio.h>  //perror

#include "msgQueue.h"

struct msgQ *initQ(int maxSize) {
  struct msgQ *queue = malloc(sizeof(struct msgQ));
  if (queue == NULL) {
    perror("malloc failed");
    exit(1);
  }
  queue->s = 0;
  queue->maxSize = maxSize;

  //setup head and tail nodes
  queue->head = malloc(sizeof(struct msgNode));
  if (queue->head == NULL) {
    perror("malloc failed");
    exit(1);
  } 
  queue->tail = malloc(sizeof(struct msgNode));
  if (queue->tail == NULL) {
    perror("malloc failed");
    exit(1);
  } 
  queue->head->prev = NULL;
  queue->tail->next = NULL;
  queue->head->next = queue->tail;
  queue->tail->prev = queue->head;

  //initialize mutex
  queue->quetex = malloc(sizeof(pthread_mutex_t));
  if (queue->quetex == NULL) {
    perror("malloc failed");
    exit(1);
  }
  pthread_mutex_init(queue->quetex,NULL);
  
  //intialize consumer condition var
  queue->condc = malloc(sizeof(pthread_cond_t));
  if (queue->condc == NULL) {
    perror("malloc failed");
    exit(1);
  }
  pthread_cond_init(queue->condc,NULL);

  //intialize producer condition var
  queue->condp = malloc(sizeof(pthread_cond_t));
  if (queue->condp == NULL) {
    perror("malloc failed");
    exit(1);
  }
  pthread_cond_init(queue->condp,NULL);

  return queue;
}

//-1 on error, 0 on success
int enqueue(struct msgQ *queue, char *msg) {
  pthread_mutex_lock(queue->quetex);
  while (queue->s == queue->maxSize) {
    pthread_cond_wait(queue->condp, queue->quetex);
  }

  struct msgNode *toAdd = malloc(sizeof(struct msgNode));
  if (toAdd == NULL) {
    perror("malloc failed");
    exit(1);
  }
  toAdd->msg = msg;

  struct msgNode *nextnext = queue->head->next;
  queue->head->next = toAdd;
  nextnext->prev = toAdd;

  toAdd->next = nextnext;
  toAdd->prev = queue->head;

  queue->s++;
  pthread_cond_signal(queue->condc);
  pthread_mutex_unlock(queue->quetex);
  return 0;
}

int skipQueue(struct msgQ *queue, char *msg) {
  pthread_mutex_lock(queue->quetex);
  while (queue->s == queue->maxSize) {
    pthread_cond_wait(queue->condp, queue->quetex);
  }

  struct msgNode *toAdd = malloc(sizeof(struct msgNode));
  if (toAdd == NULL) {
    perror("malloc failed");
    exit(1);
  }
  toAdd->msg = msg;

  struct msgNode *prevprev = queue->tail->prev;
  queue->tail->prev = toAdd;
  prevprev->next = toAdd;

  toAdd->prev = prevprev;
  toAdd->next = queue->tail;

  queue->s++;
  pthread_cond_signal(queue->condc);
  pthread_mutex_unlock(queue->quetex);
  return 0;
}

char *dequeue(struct msgQ *queue) {
  pthread_mutex_lock(queue->quetex);
  //sleep on condition variable if we can't dequeue
  while (queue->s == 0) {
    pthread_cond_wait(queue->condc, queue->quetex);
  }

  struct msgNode *toDel = queue->tail->prev;
  toDel->prev->next = queue->tail;
  queue->tail->prev = toDel->prev;

  char *msg = toDel->msg;
  free(toDel); toDel=NULL;
  queue->s--;
  pthread_cond_broadcast(queue->condp); //TODO signal vs broadcast?
  pthread_mutex_unlock(queue->quetex);
  return msg;
}

void destroyQ(struct msgQ *queue) {
  while(queue->s != 0) {
    char *msg = dequeue(queue);
    free(msg); msg = NULL;
  }
  free(queue->head); queue->head = NULL;
  free(queue->tail); queue->tail = NULL;
  //TODO check if return val not -1 on pthread calls
  pthread_mutex_destroy(queue->quetex); free(queue->quetex); queue->quetex = NULL;
  pthread_cond_destroy(queue->condc); free(queue->condc); queue->condc = NULL;
  pthread_cond_destroy(queue->condp); free(queue->condp); queue->condc = NULL;
  free(queue); queue=NULL;
}
