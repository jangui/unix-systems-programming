#include <pthread.h>      // pthread_t, pthread_create, pthread_detach
                          // pthread_self, pthread_mutex_lock
                          // pthread_mutex_unlock, pthread_exit
#include <stdio.h>        // printf, fprintf
#include <unistd.h>       // write, close
#include <stdlib.h>       // exit
#include <errno.h>        // errno
#include <string.h>       // strlen

#include "clientHandling.h" // cliendHandlerArgs
#include "clientList.h"     // clientList, client, clientAppend, clientRemove
#include "input.h"
#include "msgQueue.h"

#define MAXNAME 30    //TODO make this global?
#define MAXMSG 4096

void leaveChatRoom(char *name, int connfd, struct clientList *clients, pthread_mutex_t *connlock) {
    pthread_mutex_lock(connlock);
    clientRemove(clients, name);
    pthread_mutex_unlock(connlock);
    //TODO add "I left message to qeueu"
}

int joinChatRoom(char *name, int connfd, struct clientList *clients, pthread_mutex_t *connlock) {
    
    //try to add client unless full or name already in use
    pthread_mutex_lock(connlock);
    int status = clientAppend(clients, connfd, name);
    pthread_mutex_unlock(connlock);

    printf("%s status: %d", name, status);
    fflush(stdout);

    char *response;
    if (status != 0) {
      //JOIN FAILED
      if (status == 1) response = "NAME";
      else if (status == -1) response = "FULL";

      printf("%s\n", response);
      //report failure
      sendLine(connfd, response);
      return -1;
    }

    //JOIN SUCCESSFUL
    response = "OKAY";
    if (sendLine(connfd, response) == -1) {
      perror("failed to write to socket"); 
      return -1;
    }
    printf("%s joined chatroom", name);
    return 0;
}

void sendGreet(int fd, int onlineCount, char **names, char *name) { 
  char *greet;
  if (onlineCount == 1) {
    greet = "server: Welcome to the chat room. Looks like you're the only one here!"; 
    sendLine(fd, greet);
    return;
  }
  char *msg = "server: Welcome to the chat room. Current Members: ";   
  greet = malloc(sizeof(char*)*strlen(msg));
  if (greet == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(greet, msg);

  char *members;
  char **ptr = names;
  while(*ptr != NULL) {
    if (strcmp(*(ptr), name) == 0) {ptr++;continue;} //don't display our own name
    greet = realloc(greet, sizeof(char*)*(strlen(greet)+strlen(*ptr)+1));
    if (greet == NULL) {
      perror("realloc failed");
      exit(1);
    }
    strcat(greet, *ptr);
    strcat(greet, " ");
    ptr++;
  }
  printf("greet: %s\n", greet);
  fflush(stdout);
  sendLine(fd, greet);
  free(greet); greet = NULL;
  return;
}

char *appendName(char *name, char *message) {
  char *msg = malloc(sizeof(char*) * strlen(name)+2);
  if (msg == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(msg, name);
  strcat(msg, ": ");
  msg = realloc(msg, sizeof(char*) * (strlen(msg)+strlen(message)));
  if (msg == NULL) {
    perror("realloc failed");
    exit(1);
  }
  strcat(msg, message);
  return msg;
}

//send hello or goodbye depending on flag
void helloGoodbye(char *name, struct msgQ *queue, int flag) {
  char *server = "server: ";
  char *msg;
  if (flag == 0) { 
    msg = " has connected.";
  } else {
    msg = " has disconnected.";
  }
  int len = strlen(server)+strlen(name)+strlen(msg);
  char *helloGoodbye = malloc(sizeof(char*)*(len));
  if (helloGoodbye == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(helloGoodbye, server);
  strcat(helloGoodbye, name);
  strcat(helloGoodbye, msg);
  enqueue(queue, helloGoodbye);
  //helloGoodbye will get free'd by consumer thread
}

void *client_handler(void *a) {
  //detach ourselves
  pthread_detach(pthread_self());

  struct clientHandlerArgs *args = (struct clientHandlerArgs *) a;

  //get name from client
  printf("waiting for name...");
  fflush(stdout);
  char *name = recvLine(*(args->fd), MAXNAME);
  //TODO check if name is server or has : in it.
  printf("%s attempting connection\n", name);
  fflush(stdout);

  int status = joinChatRoom(name, *(args->fd), args->clients, args->connlock);
  if (status == -1) {
    //unable to join chatroom
    printf("%s rejected from chat room", name);
    fflush(stdout);
    free(a); a = NULL;
    free(name); name = NULL;
    return NULL;
  }
  printf("%s joined chat room\n", name);
  fflush(stdout);

  //get name of online users and send greet
  int onlineCount;
  pthread_mutex_lock(args->connlock);
  char **names = getClients(args->clients, &onlineCount);
  pthread_mutex_unlock(args->connlock);
  printf("online count: %d\n", onlineCount);
  sendGreet(*(args->fd), onlineCount, names, name);
  free(names);
  printf("greet sent to %s\n", name);
  fflush(stdout);
  if (onlineCount > 1) helloGoodbye(name, args->queue, 0);

  
  char *message;
  for (;;) {
    message = recvLine(*(args->fd), MAXMSG); 
    if (strcmp(message, "/exit") == 0) {
      free(message); message = NULL; 
      break;
    }
    char *newMsg = appendName(name, message);
    free(message), message = NULL;
    printf("%s\n", newMsg);
    fflush(stdout);
    enqueue(args->queue, newMsg); //msg gets free'd in dequeue
    //free(newMsg); newMsg = NULL;
  }

  printf("%s left\n", name);
  fflush(stdout);

  leaveChatRoom(name, *(args->fd), args->clients, args->connlock);
  helloGoodbye(name, args->queue, 1);
  printf("%s left\n", name);
  fflush(stdout);
  free(name); name = NULL;
  close(*(args->fd));
  free(args->fd); args->fd = NULL;
  free(a); a = NULL;
  return NULL;
}

void sendAll(char *msg, struct clientList *clients, pthread_mutex_t *connlock) {
  pthread_mutex_lock(connlock);
  struct client **ptr = clients->lst;
  int count = 0;
  for (int i = 0; i < clients->maxSize; i++) {
    //if we've messaged all clients no need to iterate over whole list
    if (count == clients->s) break;

    //send message to each client
    if (*ptr != NULL) {
      sendLine((*ptr)->fd, msg);
      count++;
    }
    ptr++;
  }
  pthread_mutex_unlock(connlock);
}

void *queue_handler(void *a) {
  struct queueHandlerArgs *args = (struct queueHandlerArgs *) a;

  char *msg;
  for (;;) {
    msg = dequeue(args->queue);
    sendAll(msg, args->clients, args->connlock);
    free(msg); msg = NULL;
  }
}
