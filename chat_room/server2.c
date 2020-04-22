#include <stdio.h>    //printf, fprintf, stderr
#include <stdlib.h>   //strtol, exit

#define PORT 1337   //default port 

int checkUsage(int argc, char *argv[]) {
  //usage: ./server <PORT>
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
      store our name, fd & into struct
      lock connected collection
      check if size conn== 0
        send you're only one here to client
      else
        send greet message
          hello! this is everyone connected:
      add our info struct to conn collection
      unlock conn

      loop while not received /exit:
        recieve messages
        lock queue (if space to add) (sleep on condition)
        add to message to queue
        wake up queue consumer thread
        unlock queue

      if recieved /exit:
        lock conn
        remove from conn
        unlock conn
        lock queue (if space to add) (sleep on condition)
        add this person left message to queue
        unlock queue

        close conn fd
      
      thread finishes (lets have it detached)


  queue consumer thread:
    if awaken (ready to consume)
    lock conns
    send all messages in queue to all conns
    unlock conns 
    sleep on condition var of conns

    
        
        
      
*/
