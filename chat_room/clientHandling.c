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
#include "signal.h"

//remove client from connected list
void leaveChatRoom(char *name, int connfd, struct clientList *clients, pthread_mutex_t *connlock) {
    if (pthread_mutex_lock(connlock) < 0) {
      perror("error locking mutex");
      pthread_exit((void*)1);
    }
    clientRemove(clients, name);
    if (pthread_mutex_unlock(connlock) < 0) {
      perror("error unlocking mutex");
      pthread_exit((void*)1);
    }
}

int joinChatRoom(char *name, int connfd, struct clientList *clients, pthread_mutex_t *connlock) {
    //try to add client unless full or name already in use
    if (pthread_mutex_lock(connlock) < 0) {
      perror("error locking mutex");
      pthread_exit((void*)1);
    }
    int status = clientAppend(clients, connfd, name);
    if (pthread_mutex_unlock(connlock) < 0) {
      perror("error unlocking mutex");
      pthread_exit((void*)1);
    }

    //send response to client based if they cant join or not
    char *response;
    if (status != 0) {
      //JOIN FAILED
      if (status == 1) response = "NAME";
      else if (status == -1) response = "FULL";
      printf("%s rejected from chat room: %s\n", name, response);
      //report failure
      if (sendLine(connfd, response) != 0) {
        perror("error writting to socket");
      }
      return -1;
    }

    //JOIN SUCCESSFUL
    response = "OKAY";
    if (sendLine(connfd, response) == -1) {
      perror("failed to write to socket"); 
      return -1;
    }
    return 0;
}

void sendGreet(char *name, int onlineCount, char **names, struct msgQ *queue) { 
  //send personalized greet if you're the only one in chat room
  char *greet;
  if (onlineCount == 1) {
    char *msg = "server: Welcome to the chat room. Looks like you're the only one here!\n";
    greet = malloc(sizeof(char*)*(strlen(msg)+1));
    if (greet == NULL) {
      perror("malloc failed");
      exit(1);
    }
    strcpy(greet, msg);
    enqueue(queue, greet);
    //greet gets free'd after dequeue
    return;
  }

  //normal greet
  //message must be "server:.{name} ... "
  //formatted so consumer thread only sends it appropriate client
  char *msg = "server:.";
  greet = malloc(sizeof(char*)*(strlen(msg)+strlen(name)));
  if (greet == NULL) {
    perror("malloc failed");
    exit(1);
  } 
  strcpy(greet, msg);
  strcat(greet, name);
  msg = " Welcome to the chat room. Current Members: ";   
  greet = realloc(greet, sizeof(char*)*(strlen(msg)+strlen(greet)));
  if (greet == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcat(greet, msg);

  //get online members' names and concate to greet message
  char **ptr = names;
  while(*ptr != NULL) {
    if (strcmp(*(ptr), name) == 0) {ptr++;continue;} //skip ourselves
    greet = realloc(greet, sizeof(char*)*(strlen(greet)+strlen(*ptr)+1));
    if (greet == NULL) {
      perror("realloc failed");
      exit(1);
    }
    strcat(greet, *ptr);
    strcat(greet, " ");
    ptr++;
  }
  greet = addNewLine(greet);
  enqueue(queue, greet);
  //greet free'd after dequeue'd
  return;
}

//append name + ':' to message
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
    msg = " has connected. \n";
  } else {
    msg = " has disconnected. \n";
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
  //block sigint
  sigset_t new_mask, old_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  //detach ourselves
  if (pthread_detach(pthread_self()) < 0) {
    perror("error detaching thread");
    pthread_exit((void*)1);
  }

  struct clientHandlerArgs *args = (struct clientHandlerArgs *) a;

  //get name from client
  fprintf(stderr, "Client handler assigned. Waiting for name...\n");
  char *name = recvLine(*(args->fd), MAXNAME);
  if (name == NULL) {
    perror("error reading from socket");
    pthread_exit((void*)1);
  }

  //attempt to join chat room
  int status = joinChatRoom(name, *(args->fd), args->clients, args->connlock);
  if (status == -1) {
    //unable to join chatroom
    free(a); a = NULL;
    free(name); name = NULL;
    pthread_exit((void*)1);
  }
  fprintf(stderr, "%s joined the chatroom\n", name);

  //get name of online users
  int onlineCount;
  if (pthread_mutex_lock(args->connlock) < 0) {
    perror("failed to lock mutex");
    pthread_exit((void*)1);
  }
  char **names = getClients(args->clients, &onlineCount);
  if (pthread_mutex_unlock(args->connlock) < 0) {
    perror("failed to lock mutex");
    pthread_exit((void*)1);
  }

  //queue message for client stating who is currently online
  //consumer will handle sending messages to correct person
  sendGreet(name, onlineCount, names, args->queue);
  free(names); names = NULL;

  //queue message saying we've connected
  helloGoodbye(name, args->queue, 0);
  
  //recieve messages from client
  //queue message for chatroom
  char *message;
  for (;;) {
    message = recvLine(*(args->fd), MAXLINE); 
    if (message == NULL) {
      perror("error reading from socket");
      break;
    }
    
    //if recieved "/exit" break out of loop
    if (strcmp(message, "/exit\n") == 0) {
      free(message); message = NULL; 
      break;
    }

    //queue message
    char *newMsg = appendName(name, message);
    free(message), message = NULL;
    enqueue(args->queue, newMsg); 
    //msg gets free'd once dequeue'd
  }

  //leave chat room and clean up
  leaveChatRoom(name, *(args->fd), args->clients, args->connlock);
  helloGoodbye(name, args->queue, 1);
  fprintf(stderr, "%s left the chatroom\n", name);
  free(name); name = NULL;
  close(*(args->fd));
  free(args->fd); args->fd = NULL;
  free(a); a = NULL;
  return NULL;
}

void sendAll(char *msg, struct clientList *clients, pthread_mutex_t *connlock) {
  if (pthread_mutex_lock(connlock) < 0) {
    perror("error locking mutex");
    exit(1);
  }
  struct client **ptr = clients->lst;
  int count = 0;
  for (int i = 0; i < clients->maxSize; i++) {
    //if we've messaged all clients no need to iterate over whole list
    if (count == clients->s) break;
    //send message to each client
    if (*ptr != NULL) {
      if (sendLine((*ptr)->fd, msg) != 0) {
        perror("error writting to socket");
      }
      count++;
    }
    ptr++;
  }
  if (pthread_mutex_unlock(connlock) < 0) {
    perror("error unlocking mutex"); 
    exit(1);
  }
}


//sent message to client if they are connected
void sendTo(char *name, char *msg, struct clientList *clients, pthread_mutex_t *connlock) {
  if (pthread_mutex_lock(connlock) < 0) {
    perror("error locking mutex"); 
    exit(1);
  }
  struct client **ptr = clients->lst;
  for (int i = 0; i < clients->maxSize; i++) {
    if (*ptr != NULL) {
      if (strcmp(name, (*ptr)->name) == 0) {
        if (sendLine((*ptr)->fd, msg) != 0) {
          perror("error writting to socket");
        }
        break;
      }
    }
    ptr++;
  }
  if (pthread_mutex_unlock(connlock) < 0) {
    perror("error locking mutex"); 
    exit(1);
  }
}

//find out who personalized message is to
//if their still connected, send them the message
void sendOne(char *msg, struct clientList *clients, pthread_mutex_t *connlock) {
  //msg format: "server:.{name} ..."
  //find out name and fix message for sending
  char *token = strtok(msg, " "); 
  char *front = "server:.";
  char *name = token+strlen(front);
  char *newFront = "server: ";
  char *newMsg = malloc(sizeof(char*)*(strlen(newFront)+strlen(msg+strlen(token)+1)+1));
  if (newMsg == NULL) {
    perror("malloc failed");
    exit(1);
  } 
  strcpy(newMsg, newFront);
  strcat(newMsg, msg+strlen(token)+1);
  sendTo(name, newMsg, clients, connlock);
  free(newMsg); newMsg = NULL;
  //msg gets free'd back in queue handler
}

//if message starts with "server:." then next word is the name of who to send to
//else send message to everyone
void sendCorrectly(char *msg, struct clientList *clients, pthread_mutex_t *connlock) {
  char *name = getName(msg);
  if (strcmp(name, "server") == 0) {
    if (*(msg+strlen("server:")) == '.') {
      //message from server to specific client
      sendOne(msg, clients, connlock);
      free(name); name = NULL;
      return;
    } 
  }
  sendAll(msg, clients, connlock);
  free(name); name = NULL;
}

//dequeue message, send to single person or everyone
//locks and condition vars handled by queue
void *queue_handler(void *a) {
  //block sigint
  sigset_t new_mask, old_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  struct queueHandlerArgs *args = (struct queueHandlerArgs *) a;
  char *msg;
  for (;;) {
    msg = dequeue(args->queue);
    sendCorrectly(msg, args->clients, args->connlock);
    free(msg); msg = NULL;
  }
}
