#include <stdlib.h>         // exit
#include <stdio.h>          // printf, fprintf, stderr
#include <unistd.h>         // close
#include <string.h>         // memset
#include <sys/socket.h>     // socket, AF_INET, SOCK_STREAM, send, recv
#include <arpa/inet.h>      // inet_pton
#include <netinet/in.h>     // servaddr
#include <errno.h>          // errno
#include <sys/select.h>     // select, fd_set, FD_ZERO, FD_SET

#include "input.h"          // getLine, recvLine, MAXLINE

#define PORT 1337
#define IPADDR "127.0.0.1"

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

// establish connection with if there is space
void joinChatRoom(int connfd) {
  int n;

  //recieve response from server
  char *response = recvLine(connfd, 5);

  if (strcmp(response, "OKAY") == 0) {
    printf("Connection Established"); 
  } else if (strcmp(response, "FULL") == 0) {
    printf("**Chat room full. Please try again later.**\n");
    exit(1);
  } else if (strcmp(response, "NAME") == 0) {
    printf("Name in use. Please connect again with a different name.\n");
    exit(1);
  } else {
    printf("Failed to connect.\n"); 
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

  printf("Sending server our name... \n");
  //sendServer our name
  if(sendLine(connfd, argv[1]) == -1) {
    perror("failed to write to socket");
    exit(1);
  }
  printf("Name sent.");

  printf("Attempting to join chatroom...");
  fflush(stdout);
  //check if server let us join chat room
  joinChatRoom(connfd);
  printf("joined chatroom");
  fflush(stdout);

  //setup set for pselect
  fd_set fds;
  FD_ZERO(&fds);
  int fd;

  //once connected, continously send and recieve lines
  char *recvline;
  char *message;
  for (;;) {
    FD_SET(0, &fds);
    FD_SET(connfd, &fds);
     
    fd = select(connfd+1, &fds, NULL, NULL, NULL);
    
    if (fd < 0) {perror("select failed");exit(1);}

    if (FD_ISSET(0, &fds)) {
      //getLine from stdin & send to server
      message = getLine(MAXLINE);
      sendLine(connfd, message);
      //check if we sent "/exit", if so break out of loop
      if (strcmp(message, "/exit") == 0) {free(message); break;}
      free(message); message= NULL;
    }

    if (FD_ISSET(connfd, &fds)) {
      //receive line from server and print
      recvline = recvLine(connfd, MAXLINE);
      if(recvline == NULL) {
         perror("read failed");
         exit(1);
      }
      //check if received exit from server
      if (strcmp(recvline, "/exit") == 0) break;
    }
  }
  close(connfd);
  printf("Disconected.\n");
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
        
    optional:
    change getLine to getline regardless of line length
*/
