#include <unistd.h>       // close
#include <stdio.h>        // fprint, stderr
#include <stdlib.h>       // exit
#include <string.h>       // memset, strlen
#include <errno.h>        // errno
#include <netinet/in.h>   // servaddr, INADDR_ANY, htons, htonl
#include <sys/select.h>   // select, fd_set, FD_ZERO, FD_SET
#include <sys/socket.h>   // socket, AF_INET, SOCK_STREAM
                          // bind, liste, accept, send, recv

#include "input.h"        // getLine

#define MAXNAME    30     // max length of name
#define PORT       1337   // default port 
#define LISTENQ    10     // maximum q'd connections
#define TIMEOUT    30     // timeout time for select

//check usage and setup default vals
void usage(int argc, char *argv[], int *port) {
  //usage check
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: %s Name <Port>\n", argv[0]);
    exit(errno);
  }
  if (argc == 3) {
    *port = strtol(argv[2], &argv[2], 10);
  } else {
    *port = PORT; 
  }
}

void recvLine(char *clientName, int connfd, char recvline[]) {
  int n;
  //readline from client & print
  printf("%s: ", clientName);
  if ((n = read(connfd, recvline, MAXLINE)) == -1) {
    perror("failed to read from socket");
    exit(errno);
  }
  recvline[n] = '\0';
  if (fputs(recvline, stdout) == EOF) {
    fprintf(stderr,"fputs Error\n");
    exit(errno);
  }
  printf("\n");
  fflush(stdout);
}

//send message to 
void sendMsg(char **message, int connfd) {
  *message = getLine();
  if (write(connfd, *message, strlen(*message)) == -1) {
    perror("fail to write to socket");
    exit(errno);
  }
}

//send "/exit" on timeout
void timeoutExit(int connfd) {
  if (write(connfd, "/exit", strlen("/exit")) == -1) {
    perror("fail to write to socket");
    exit(errno);
  }
  fprintf(stderr, "timeout\n");
  exit(2);
}

//do setup w/ client for chat
void handshake(int connfd, char clientName[], char *name) {
  int n;
  //receive client's name
  printf("Receiving name from client...\n");
  if ((n = read(connfd, clientName, MAXNAME)) == -1) {
      fprintf(stderr, "read error\n");
      exit(errno);
  }
  clientName[n] = '\0';

  //send our name to client
  printf("Sending client our name...\n");
  if (write(connfd, name, strlen(name)) < 0) {
    perror("failed to write to socket");
    exit(errno);
  } 
  printf("Connected to %s\n\n", clientName);
}

//socket, bind, listen, accept
int main(int argc, char** argv) {
  int port;

  //check usage and setup default vals
  usage(argc, argv, &port);

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
  
  //setup set for pselect
  fd_set fds;
  FD_ZERO(&fds);
  struct timeval timeout;
  int fd;

  //continously wait for a connection
  int connfd, n;
  char clientName[MAXNAME+1];
  for (;;) { 
    //wait for a connection
    connfd = accept(socketfd, NULL, NULL);
    printf("Client connecting...\n");

    //setup w/ client for chat
    handshake(connfd, clientName, argv[1]);

    //chat loop
    char recvline[MAXLINE + 1];
    char *message;
    for (;;) {
      timeout.tv_sec = TIMEOUT;
      timeout.tv_usec = 0;
      FD_SET(0, &fds);
      FD_SET(connfd, &fds);

      fd = select(connfd+1, &fds, NULL, NULL, &timeout);
      
      if (fd < 0) {perror("select failed");exit(1);}
      if (fd == 0) timeoutExit(connfd);

      if (FD_ISSET(0, &fds)) {
        //getLine from stdin & send to client
        sendMsg(&message, connfd);
        //check if we sent "/exit"
        if (strcmp(message, "/exit") == 0) {free(message); break;}
        free(message);
      }

      if (FD_ISSET(connfd, &fds)) {
        //recieve line from client and print
        recvLine(clientName, connfd, recvline);
        //check if recieved exit
        if (strcmp(recvline, "/exit") == 0) break;
      }
    }
    printf("Disconnecting.\n\n");
    fflush(stdout);
    close(connfd);
  }
  close(socketfd);
  return 0;
}
