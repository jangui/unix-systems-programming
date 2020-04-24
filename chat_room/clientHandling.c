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
      free(response); response = NULL;
      return -1;
    }
    free(response); response = NULL;
    return 0;
}

void sendGreet(int fd, int onlineCount, char **names) {
  char *greet;
  if (onlineCount == 1) {
    greet = "Server: Welcome to the chat room. Looks like you're the only one here!"; 
    sendLine(fd, greet);
    return;
  }
  char *msg = "Server: Welcome to the chat room. Current Members: ";   
  greet = malloc(sizeof(char*)*strlen(msg));
  if (greet == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(greet, msg);

  char *members;
  char **ptr = names;
  while(*ptr != NULL) {
    greet = realloc(greet, sizeof(char*)*(strlen(greet)+strlen(*ptr)+1));
    if (greet == NULL) {
      perror("realloc failed");
      exit(1);
    }
    strcat(greet, *ptr);
    strcat(greet, " ");
    ptr++;
  }
  sendLine(fd, greet);
  free(greet); greet = NULL;
  return;
}

void appendName(char *name, char *message) {
  char *n2 = malloc(sizeof(char*) * strlen(name)+2);
  strcpy(n2, name);
  strcat(n2, ": ");
  message = realloc(message, sizeof(char*) * (strlen(n2)+strlen(message)));
  strcat(message, n2);
  free(n2); n2 = NULL;
}

void *client_handler(void *a) {
  //detach ourselves
  pthread_detach(pthread_self());

  struct clientHandlerArgs *args = (struct clientHandlerArgs *) a;

  //get name from client
  printf("waiting for name...");
  fflush(stdout);
  char *name = recvLine(args->fd, MAXNAME);
  printf("%s attempting connection\n", name);
  fflush(stdout);

  int status = joinChatRoom(name, args->fd, args->clients, args->connlock);
  if (status == -1) {
    //unable to join chatroom
    free(a); a = NULL;
    free(name); name = NULL;
    return NULL;
  }

  //get name of online users and send greet
  int onlineCount;
  char **names = getClients(args->clients, &onlineCount);
  sendGreet(args->fd, onlineCount, names);
  free(names);
  
  char *message;
  for (;;) {
    message = recvLine(args->fd, MAXMSG); 
    appendName(name, message);
    //add message to queue
    free(message); message = NULL;
  }

  leaveChatRoom(name, args->fd, args->clients, args->connlock);
  free(name); name = NULL;
  free(a); a = NULL;
  return NULL;
}

