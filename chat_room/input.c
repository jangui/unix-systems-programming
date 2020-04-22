#include <stdlib.h> // exit, malloc, free, EOF
#include <errno.h>  // errno
#include <string.h> // strtok
#include <unistd.h> // read
#include <stdio.h>  // printf, perror, fprintf
#include <string.h> // strtok
#include "input.h"

char *getLine() {
  //malloc char * which we will return with input from stdin
  char *input = malloc((sizeof(char) * MAXLINE));
  if (input == NULL) {perror("malloc failed\n");exit(errno);}

  //read a character at a time
  int pos = 0;
  int readFlag = read(0, input, 1);
  while (readFlag > 0) {
   pos++;

   //if reading more than MAXLINE, fail
   if (pos > MAXLINE) {
     fprintf(stderr, "error: message length too long ");
     fprintf(stderr, "max length: %d\n", MAXLINE);
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

