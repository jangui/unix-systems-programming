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

#include "clientList.h"      // clientVector, MAXCONS
#include "clientHandling.h"  // clientCount, clientCountMutex
                             // client_handler, establishConnection, clientHandlerArgs
#include "msgQueue.h"

#define PORT 1337        // default port
#define LISTENQ 10       // max size for connection queue
#define MAXCONS 3        // max numbers of connections
#define MAXQ 3

pthread_mutex_t connlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;

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


int main(int argc, char *argv[]) {
  int port = checkUsage(argc, argv);

  //create socket, ipv4 using tcp
  int socketfd;
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

  //TODO startchat() func with all dis below

  //create shared data structures
  struct msgQ *queue = initQ(MAXQ);
  struct clientList *clients = initClientList(MAXCONS);

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
  pthread_t tid_consumer;
  if (pthread_create(&tid_consumer, NULL, queue_handler, (void*)args) < 0) {
    perror("error making thread");
    exit(1);
  }

  //continously loop assigning handlers to each incoming connection
  int connfd;
  while (connfd = accept(socketfd, NULL, NULL)) { 

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
  return 0;
}

/*
  DESIGN

  other:
    array for structs of fd and names (2 arrys ? one for fd one for name?)
      ability to grow / shrink array dynamically
        arry grows doubles when capacity more than half full
        halves when 1/8 full
          might be gaps in arry so careful with copying to new arry
      add / remove elements from the array based on name
        add should go into first empty spot
      lookups based on name

  server should handle sigint or sigquit:
    lock conns 




  per client thread: (queue producer thread)
    init w/ client
      client sends name, we recieve
        if name taken, req other name
          lookup by name needed
      store our name, fd & into struct
      lock connected collection
      check if size conn== 0
        send you're only one here to client
      else
        send greet message
          hello! this is everyone connected:
      add our info struct to conn collection
      unlock conn

     while not received /exit:
        recieve messages
        lock queue (if space to add) (sleep on condition)
        add to message to queue
          -concat our name to message
        wake up queue consumer thread
        unlock queue

      if recieved /exit:
        lock conn
        remove from conn
          -remove based on fd
        unlock conn
        lock queue (if space to add) (sleep on condition)
        add "x person left message" to queue
        unlock queue

        close conn fd
      
      thread finishes (lets have it detached)


  queue consumer thread:
    if awaken (ready to consume)
    lock conns
    send all messages in queue to all conns
    unlock conns 
    sleep on condition var of conns

    
  clean up thread:
    inf loop call to join() 
  
  or use detached threads
    when server wants to exit
      make all threads terminate
        
      




*/
