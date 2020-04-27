#include <stdlib.h> // exit, malloc, free, EOF
#include <errno.h>  // errno
#include <string.h> // strtok
#include <unistd.h> // read
#include <stdio.h>  // printf, perror, fprintf, fputs, fflush, stdout
#include <string.h> // strtok


char *recvLine(int fd, int maxline) {
  int n;
  char *recvline = malloc(sizeof(char*)*maxline+1);
  recvline[maxline] = '\0';
  if ((n = read(fd, recvline, maxline)) == -1) {
    perror("failed to read from socket");
    return NULL;
  }
  recvline[n] = '\0';
  return recvline;
}

//get line from stdin
char *getLine(int maxline) {
  //malloc char * which we will return with input from stdin
  char *input = malloc((sizeof(char) * maxline));
  if (input == NULL) {perror("malloc failed\n");exit(errno);}

  //read a character at a time
  int pos = 0;
  int readFlag = read(0, input, 1);
  while (readFlag > 0) {
   pos++;

   //if reading more than maxline, fail
   if (pos > maxline) {
     fprintf(stderr, "error: message length too long ");
     fprintf(stderr, "max length: %d\n", maxline);
     exit(1);
   }

   //read a character at a time
   readFlag = read(0, input+pos, 1);

   //if character new line or EOF replace with null and return
   if (input[pos] == '\n' || input[pos] == EOF) {input[pos] = '\0'; return input;}
  }

  //if read failed exit with error
  if (readFlag == -1) {
    perror("read failed");
    exit(errno);
  }
}

int sendLine(int fd, char *msg) {
    if (write(fd, msg, strlen(msg)) < 0) {
      perror("failed to write to socket");
      return -1;
    }
    return 0;
}

//return name from message recieved
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

char *addNewLine(char *msg) {
  char *newMsg = malloc(sizeof(char*)*strlen(msg)+2);
  if (newMsg == NULL) {
    perror("malloc failed");
    exit(1);
  }
  strcpy(newMsg, msg);
  strcat(newMsg, "\n");
  free(msg); msg = NULL;
  return newMsg;
}

