#include <stdio.h>        // perror
#include <stdlib.h>       // malloc, calloc, realloc
#include <string.h>       // strcmp

#include "clientList.h"   // clientList, client

struct clientList *initClientList(int maxSize) {
  struct clientList *clients = malloc(sizeof(struct clientList));
  if (clients == NULL) {
    perror("malloc failed");
    exit(1);
  }
  clients->s = 0;
  clients->maxSize = maxSize;
  clients->lst = calloc(maxSize, sizeof(struct client*));
  if (clients->lst == NULL) {
    perror("malloc failed");
    exit(1);
  }
  return clients;
}

//check if client in list or not
//0 if found, -1 if not
int lookupClient(struct clientList *clients, char *name) {
  struct client **ptr = clients->lst;
  for (int i = 0; i < clients->maxSize; i++) {
    if (*ptr != NULL) {
      if (strcmp((*ptr)->name, name) == 0) {
        return 0;
      }
    }
    ptr++;
  }
  return -1;
}

//return 0 if add successful, -1 if list full, 1 if name already in use
int clientAppend(struct clientList *clients, int fd, char *name) {
  // check if space
  if (clients->s == clients->maxSize) return -1;
  if (lookupClient(clients, name) == 0) return 1;

  // make client struct
  struct client *c = malloc(sizeof(struct client));
  if (c == NULL) {
    perror("malloc failed");
    exit(1);
  }
  c->fd = fd;
  c->name = name;
    
  //knowing theres space insert into first spot available
  struct client **ptr = clients->lst;
  for (int i = 0; i < clients->maxSize; i++) {
    if (*ptr == NULL) {
      *ptr = c;    
      break;
    }
    ptr++;
  }
  clients->s++;
  return 0;
}

//returns 0 if removed, -1 otherwise
//search based on name (each node's name is unique)
int clientRemove(struct clientList *clients, char *name) {
  struct client **ptr = clients->lst;
  for (int i = 0; i < clients->maxSize; i++) {
    if (*ptr != NULL) {
      if (strcmp((*ptr)->name, name) == 0) {
        (*ptr)->name = NULL;
        free(*ptr);
        *ptr = NULL;
        clients->s--;
        return 0;
      }
    }
    ptr++;
  }
  return -1;
}


// get all client's names 
char **getClients(struct clientList *clients, int *c) {
  *c = 0;
  char **names = calloc(clients->s+1, sizeof(char*));
  if (names == NULL) {
    perror("malloc failed");
    exit(1);
  }

  //zero out first and last
  names[0] = NULL;
  names[clients->s] = NULL;

  //get each name
  struct client **ptr = clients->lst;
  int count = 0;
  for (int i = 0; i < clients->maxSize; i++) {
    //if we've collected all names before traversing full list return names
    if (count == clients->s) break;

    // if current spot is being used, get name
    if (*ptr != NULL)  {
      names[count] = (*ptr)->name;
      count++;
      *c++;
    }
    ptr++;
  }
  return names;
}
