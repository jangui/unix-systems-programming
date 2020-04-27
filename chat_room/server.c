#include <stdio.h>        // printf, fprintf, stderr
#include <stdlib.h>       // strtol, exit
#include <netinet/in.h>   // servaddr, INADDR_ANY, htons, htonl
#include <sys/socket.h>   // socket, bind, listen, accpet, AF_INET
                          // SOCK_STREAM
#include <errno.h>        // errno
#include <string.h>       // memset
#include <pthread.h>      // pthread_t, pthread_create, pthread_mutex_lock
                          // pthread_mutex_unlock
#include <unistd.h>       // close
#include <signal.h>       // sigaction, sigset_t, sigprocmask

#include "clientList.h"      // clientVector, MAXCONS
#include "clientHandling.h"  // clientCount, clientCountMutex
                             // client_handler, establishConnection, clientHandlerArgs
#include "msgQueue.h"

#define _XOPEN_SOURCE

#define PORT 1337         // default port
#define LISTENQ 10        // max size for connection queue
#define MAXCONS 15        // max numbers of connections
#define MAXQ 5            // max number of messages that can be in queue

pthread_mutex_t connlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;

//globals required for signal handler
pthread_t tid_consumer;      // queue handler thread id
int socketfd;                // socket fd
struct msgQ *queue;          // message queue
struct clientList *clients;  // connection list of clients

void safeExit(int signum);
int checkUsage(int argc, char *argv[]);
void startChat();

int main(int argc, char *argv[]) {
  //signal handling
  struct sigaction new_action, old_action;
  sigemptyset(&new_action.sa_mask);
  sigaddset(&new_action.sa_mask, SIGINT);
  new_action.sa_handler = safeExit;
  sigaction(SIGINT, &new_action, &old_action);

  //check usage
  int port = checkUsage(argc, argv);

  //create socket, ipv4 using tcp
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error");
    exit(errno);
  }
  
  //setup sockaddr_in
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  //bind
  bind(socketfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  //listen
  listen(socketfd, LISTENQ);

  startChat();

  return 0;
}

void startChat() {
  //create queue and client list
  queue = initQ(MAXQ);
  clients = initClientList(MAXCONS);

  //setup args for queue handler (consumer)
  struct queueHandlerArgs *args = malloc(sizeof(struct queueHandlerArgs));
  if (args == NULL) {
    perror("malloc failed");
    exit(1);
  }
  args->queue = queue;
  args->connlock = &connlock;
  args->clients = clients;

  //start consumer thread
  if (pthread_create(&tid_consumer, NULL, queue_handler, (void*)args) < 0) {
    perror("error making thread");
    exit(1);
  }

  //continously loop assigning handlers to each incoming connection
  int connfd;
  while ((connfd = accept(socketfd, NULL, NULL))) { 

    //setup args for client handler (producer)
    struct clientHandlerArgs *args = malloc(sizeof(struct clientHandlerArgs));
    if (args == NULL) {
      perror("malloc failed");
      exit(1);
    }
    args->queue = queue;
    args->connlock = &connlock;
    args->clients = clients;
    int *client_fd = malloc(sizeof(int));
    if (client_fd == NULL) {
      perror("malloc failed");
      exit(1);
    }
    *client_fd = connfd;
    args->fd = client_fd;

    //start client handler thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, client_handler, (void*)args) < 0) {
      perror("error making thread");
      exit(1);
    }
  }
  if (connfd < 0) {
    perror("connection failed");
    exit(1);
  }
}

// usage: ./server <PORT>
int checkUsage(int argc, char *argv[]) {
  if (argc == 1) return PORT;

  char *endptr;
  int port = strtol(argv[1], &endptr, 10);
  if (argv[1] == endptr || argc > 2) {
    fprintf(stderr, "usage: %s <PORT>\n", argv[0]);
    exit(1);
  }

  if (port < 1024 || port > 65535) {
    fprintf(stderr, "invalid port\n");
    exit(1);
  }
  return port;
}

//signal handler for sigint
void safeExit(int signum) {
  //if we haven't started accepting connections
  //close socket and exit
  if (queue == NULL && clients == NULL) {
    close(socketfd);
    exit(0);
  }    
  //exit message needs to be on heap
  char *ext = "server: /exit\n";
  char *msg = malloc(sizeof(char*)*(strlen(ext)+1));
  if (msg == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(msg, ext);
  //skip queue exit message
  //this will cause all clients to response with /exit message
  //causing client threads to terminate gracefully
  //new clients cant connect (accepting thread is sigint handler)
  skipQueue(queue, msg);

  //ask consumer thread to terminate
  if (pthread_cancel(tid_consumer) < 0) {
    perror("error canceling thread");
    exit(1);
  }

  //terminate
  sleep(1); //sleep a sec to make sure all clients disconnected
  close(socketfd);
  exit(0);
}
