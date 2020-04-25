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

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

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
  char *response = recvLine(connfd, 4);

  if (strcmp(response, "OKAY") == 0) {
    printf(GRN "Connection Established\n" RESET); 
  } else if (strcmp(response, "FULL") == 0) {
    printf(RED "**Chat room full. Please try again later.**\n");
    exit(1);
  } else if (strcmp(response, "NAME") == 0) {
    printf(RED "Name in use. Please connect again with a different name.\n");
    exit(1);
  } else {
    printf(RED "Failed to connect.\n"); 
    exit(1);
  }
}

char *getName(char *msg) {
  char *name = malloc(sizeof(char*)*(strlen(msg)));
  if (name == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(name, msg);
  strtok(name, ":");
  name = realloc(name, sizeof(char*)*strlen(name));
  if (name == NULL) {
    perror("realloc failed");
    exit(1);
  }
  return name;
}

void displayServerMsg(char *message, char *name) {
  //"strip" server name from msg
  char *serverName = "server: ";
  int msgLen = strlen(message)-strlen(serverName);
  char *msg = malloc(sizeof(char*)*msgLen);
  if (msg == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(msg, message + strlen(serverName));

  //if server sends us message starting w/ our name, we ignore
  //(message saying we have connected)
  int len = strlen(name)+1;
  char *n = malloc(sizeof(char*)*len);
  //copy start of message received and split by space
  strncpy(n, msg, len);
  strtok(n, " ");
  if (strcmp(n, name) == 0) {
    printf("ignored message from us\n");
    free(n); n = NULL;
    free(msg); msg = NULL;
    return;
  }
  free(n); n = NULL;
  //we have to do it this way to get around
  //names that are substrings in other names


  //print connection messages in green, disconnect in red
  // & welcome in blue
  if (strstr(msg, "disconnected") != NULL) {
    printf(RED "%s\n" RESET, msg); 
  } else if (strstr(msg, "connected") != NULL) {
     printf(GRN "%s\n" RESET, msg); 
  } else {
   printf(BLU "%s\n" RESET, msg); 
  }
  fflush(stdout);
  free(msg); msg = NULL;

}

void displayMessage(char *msg, char *ourName) {
    //if message comes from server display appropriately
    char *name = getName(msg); 
    if (strcmp(name, "server") == 0) {
      displayServerMsg(msg, ourName); 
    } else if (strcmp(name, ourName) == 0) {
        //if message from ourselves, ignore
        ;
    } else {
      printf("%s\n", msg);
      fflush(stdout);
    }
    free(name); name = NULL;
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
  printf("joined chatroom\n");
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
      //else print line
      displayMessage(recvline, argv[1]);
      free(recvline); recvline = NULL;
    }
  }
  close(connfd);
  printf(RED "Disconected.\n");
  return 0;
}

/*
 * TODO
 * dont print what comes in from stdin
          optional: replace name with you if your the one who sent message

     optional:
     deal wtih sigint or sigquit
      send /exit to server
      close fd
      exit
        
    optional:
    change getLine to getline regardless of line length
*/
