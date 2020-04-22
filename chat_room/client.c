#include <stdlib.h>         // exit
#include <stdio.h>          // printf, fputs, fprintf, stderr
#include <unistd.h>         // close
#include <string.h>         // memset
#include <sys/socket.h>     // socket, AF_INET, SOCK_STREAM, send, recv
#include <arpa/inet.h>      // inet_pton
#include <netinet/in.h>     // servaddr
#include <errno.h>          // errno
#include <sys/select.h>     // select, fd_set, FD_ZERO, FD_SET

#include "input.h"          // getLine

#define MAXNAME   30           // max length of name
#define	BUFFSIZE  8192	       // buffer size for reads and writes 
#define PORT      1337         // default port to connect to 
#define IPADDR    "127.0.0.1"  // default ip address to connect to
#define TIMEOUT   30

//check usage and setup default vars
void usage(int argc, char *argv[], char **ipaddr, int *port) {
  //usage check
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "usage: %s Name <IPaddress> <Port>\n", argv[0]);
    exit(errno);
  }
  //set default params
  if (argc >= 3) {
    *ipaddr = argv[2];
  } else {
    *ipaddr = IPADDR;
  }
  if (argc == 4) {
    *port = strtol(argv[3], &argv[3], 10);  
  } else {
    *port = PORT;
  }
}

//get message from server & print
void recvLine(int connfd, char recvline[], int maxline) {
  int n;
  if ((n = read(connfd, recvline, maxline)) == -1) {
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

void setName(int connfd, char *name) {
  //send server our name
  printf("Sending server our name... \n");
  if (write(connfd, name, strlen(name)) < 0) {
    perror("failed to write to socket");
    exit(errno);
  }
  //recieve OK or NO from server
  //OK means name is available, NO means its in use
  char response[2];
  printf("Waiting for server's response...\n");
  int n;
  if ((n = read(connfd, response, 2)) < 0) {
    perror("read error");
    exit(errno);
  }
  response[n] = '\0';
  if (strcmp(response, "OK") != 0) {
    printf("Server: Name \"%s\" in use\n", name);
    printf("Please connect with a different name.\n");
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  int port;
  char *ipaddr;

  //check usage and set default vals
  usage(argc, argv, &ipaddr, &port);

  //make socket
  int connfd;  
  if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "socket error");
      exit(errno);
  }

  //setup for connect
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(port);
  if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0) {
      fprintf(stderr, "inet_pton error for: %s\n", ipaddr);
      exit(errno);
  }

  //connect
  if (connect(connfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
    exit(errno);
  }

  //set a valid name with the server
  setName(connfd, argv[1]);

  //setup set for pselect
  fd_set fds;
  FD_ZERO(&fds);
  int fd;

  //once connected, continously send and recieve lines
  char recvline[MAXLINE + 1];
  char *message;
  for (;;) {
    FD_SET(0, &fds);
    FD_SET(connfd, &fds);
     
    fd = select(connfd+1, &fds, NULL, NULL, NULL);
    
    if (fd < 0) {perror("select failed");exit(1);}
    if (fd == 0) {
      if (write(connfd, "/exit", strlen("/exit")) == -1) {
        perror("fail to write to socket");
        exit(errno);
      }
      fprintf(stderr, "timeout\n");exit(2);
    }

    if (FD_ISSET(0, &fds)) {
      //getLine from stdin & send to server
      message = getLine();
      if (write(connfd, message, strlen(message)) == -1) {
        perror("failed to talk to server");
        exit(errno);
      }
      //check if we sent "/exit"
      if (strcmp(message, "/exit") == 0) {free(message); break;}
      free(message);
    }

    if (FD_ISSET(connfd, &fds)) {
      //receive line from server and print
      recvLine(connfd, recvline, MAXLINE);
      //check if received exit
      if (strcmp(recvline, "/exit") == 0) break;
    }
  }
  printf("Disconnecting\n");
  close(connfd);
  return 0;
}

/*
  DESIGN:
  x  conncet to server
      loop until name accepted: (done without loop for now)
  x      send name
  x      server replies
  x        rejected
  x          loop
  x        accepted
  x          exit loop
  x    poll stdin and conn fd for input
  x      stdin
  x        recieve message from stdin
  x        send to server
  x      conn fd
          optional) deal with cleanring stdin & reseting it when mssg recv
  x        print message on screen
          optional: replace name with you if your the one who sent message

     optional:
     deal wtih sigint or sigquit
      send /exit to server
      close fd
      exit
        
*/
