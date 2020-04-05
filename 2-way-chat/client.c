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
void recvLine(char *serverName, int sockfd, char recvline[]) {
  int n;
  printf("%s: ", serverName);
  if ((n = read(sockfd, recvline, MAXLINE)) == -1) {
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

void handshake(int sockfd, char serverName[], char *name) {
  //send server our name
  printf("Sending server our name... \n");
  if (write(sockfd, name, strlen(name)) < 0) {
    perror("failed to write to socket");
    exit(errno);
  }

  //recieve server's name
  printf("Receiving name from server...\n");
  int n;
  if ((n = read(sockfd, serverName, MAXNAME)) < 0) {
    perror("read error");
    exit(errno);
  }
  serverName[n] = '\0';
  printf("Connected to %s\n\n", serverName);
}

int main(int argc, char* argv[]) {
  int port;
  char *ipaddr;

  //check usage and set default vals
  usage(argc, argv, &ipaddr, &port);

  //make socket
  int sockfd;  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "socket error");
      exit(errno);
  }

  //setup for connect
  struct sockaddr_in	servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(port);
  if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0) {
      fprintf(stderr, "inet_pton error for: %s\n", ipaddr);
      exit(errno);
  }

  //connect
  if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
    exit(errno);
  }

  //setup w/ server for chat
  char serverName[MAXNAME+1];
  handshake(sockfd, serverName, argv[1]);

  //setup set for pselect
  fd_set fds;
  FD_ZERO(&fds);
  struct timeval timeout;
  int fd;

  //once connected, continously send and recieve lines
  char recvline[MAXLINE + 1];
  char *message;
  for (;;) {
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    FD_SET(0, &fds);
    FD_SET(sockfd, &fds);
     
    fd = select(sockfd+1, &fds, NULL, NULL, &timeout);
    
    if (fd < 0) {perror("select failed");exit(1);}
    if (fd == 0) {
      if (write(sockfd, "/exit", strlen("/exit")) == -1) {
        perror("fail to write to socket");
        exit(errno);
      }
      fprintf(stderr, "timeout\n");exit(2);
    }

    if (FD_ISSET(0, &fds)) {
      //getLine from stdin & send to server
      message = getLine();
      if (write(sockfd, message, strlen(message)) == -1) {
        perror("failed to talk to server");
        exit(errno);
      }
      //check if we sent "/exit"
      if (strcmp(message, "/exit") == 0) {free(message); break;}
      free(message);
    }

    if (FD_ISSET(sockfd, &fds)) {
      //receive line from server and print
      recvLine(serverName, sockfd, recvline);
      //check if received exit
      if (strcmp(recvline, "/exit") == 0) break;
    }
  }
  printf("Disconnecting\n");
  close(sockfd);
  return 0;
}
