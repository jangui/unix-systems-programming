#include <stdlib.h>         // exit
#include <stdio.h>          // printf, fprintf, stderr
#include <unistd.h>         // close
#include <string.h>         // memset
#include <sys/socket.h>     // socket, AF_INET, SOCK_STREAM, send, recv
#include <arpa/inet.h>      // inet_pton
#include <netinet/in.h>     // servaddr
#include <errno.h>          // errno
#include <sys/select.h>     // select, fd_set, FD_ZERO, FD_SET
#include <signal.h>

#include "input.h"          // getLine, recvLine, MAXLINE

#define _XOPEN_SOURCE

#define PORT 1337
#define IPADDR "127.0.0.1"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

//need socket fd for signal handler
int *fdptr = NULL;

//signal handler for sigint
void safeExit(int signum) {
  if (fdptr == NULL) {
    exit(1); 
  }
  sendLine(*fdptr, "/exit\n");
  printf(RED "Disconnected.\n"); 
  exit(0);
}

//names cannot inlcude spaces, periods, or colons.
//name cannot be "server"
void validateName(char *name) {
  int valid = 1;
  if (strstr("server", name) != NULL) {
    valid = 0;
  } else if (strchr(name, '.') != NULL) {
    valid = 0;
  } else if (strchr(name, ':') != NULL) {
    valid = 0;
  } else if (strchr(name, ' ') != NULL) {
    valid = 0;
  }
  if (valid == 0) {
    printf(RED "Invalid name. Name cannot be \"server\" or include ");
    printf("periods, colons or spaces.\n");
    exit(1);
  }
}

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

// handle response from server for joining chat room
// if rejected, execution terminates
void joinChatRoom(int connfd) {
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

//displays messages from server appropriately
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
  //check if first word in message is our name
  strncpy(n, msg, len);
  strtok(n, " ");
  if (strcmp(n, name) == 0) {
    free(n); n = NULL;
    free(msg); msg = NULL;
    return;
  }
  free(n); n = NULL;

  //connection messages: green, disconnect: red, welcome: blue
  if (strstr(msg, "disconnected.") != NULL) {
    printf(RED "%s\n" RESET, msg); 
  } else if (strstr(msg, "connected.") != NULL) {
     printf(GRN "%s\n" RESET, msg); 
  } else {
   printf(BLU "%s\n" RESET, msg); 
  }
  fflush(stdout); //make sure color gets reset
  free(msg); msg = NULL;
}

void displayMessage(char *msg, char *ourName) {
    //if two messages got jumbled in same recvline
    //display them seperately
    int len = strlen(msg);
    for (int i = 1; i < len; i++) {
      if (*(msg+i) == '\n') {
        *(msg+i) = '\0';
        if (i == len-1) break; //last message to be read
        displayMessage(msg+i+1, ourName); 
      }
    }

    //check if received exit from server
    if (strcmp(msg, "server: /exit") == 0) {
      printf(RED "Server Disconected.\n");
      close(*fdptr); 
      exit(1);
    }

    //check who message is coming from
    char *name = getName(msg); 
    if (strcmp(name, "server") == 0) {
      //display server messages appropriately
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

  //signal handling
  struct sigaction new_action, old_action;
  sigemptyset(&new_action.sa_mask);
  sigaddset(&new_action.sa_mask, SIGINT);
  new_action.sa_handler = safeExit;
  sigaction(SIGINT, &new_action, &old_action);

  //validate name
  validateName(argv[1]);

  //make socket
  int connfd;  
  if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "socket error");
      exit(errno);
  }
  fdptr = &connfd;

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

  //send server our name
  fprintf(stderr, "Sending server our name... \n");
  if(sendLine(connfd, argv[1]) == -1) {
    perror("failed to write to socket");
    exit(1);
  }

  //check if server let us join chat room
  fprintf(stderr, "Attempting to join chatroom...\n");
  joinChatRoom(connfd);

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
      message = addNewLine(message);
      sendLine(connfd, message);
      //check if we sent "/exit", if so break out of loop
      if (strcmp(message, "/exit\n") == 0) {free(message); break;}
      free(message); message= NULL;
    }

    if (FD_ISSET(connfd, &fds)) {
      //receive line from server and print
      recvline = recvLine(connfd, MAXLINE);
      if(recvline == NULL) {
         perror("read failed");
         exit(1);
      }
      //else print line
      displayMessage(recvline, argv[1]);
      free(recvline); recvline = NULL;
    }
  }
  close(connfd);
  printf(RED "Disconected.\n");
  return 0;
}
