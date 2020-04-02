#include <stdlib.h> // exit, malloc, free, EOF
#include <errno.h>  // errno
#include <string.h> // strtok
#include <unistd.h> // read
#include <stdio.h>  // printf, perror, fprintf
#include <string.h> // strtok
#include "input.h"

#define ARG_MAX 2000

char *getLine() {
  char *input = malloc((sizeof(char) * ARG_MAX));
  if (input == NULL) {perror("malloc failed\n");exit(errno);}
  int pos = 0; 
  //read character a character at a time
  int readFlag = read(0, input, 1);
  while (readFlag > 0) {
   pos++;
   //if reading more than arg_max fail
   if (pos > ARG_MAX) {
     fprintf(stderr, "error: command length too long\n");
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

struct command* line2arr(char *line) {
  struct command *c = malloc(sizeof(struct command));
  if (c == NULL) {perror("malloc failed\n");exit(errno);}
  //devide line by spaces and count how many
  c->len = 0;
  if (strtok(line, " ") != NULL) c->len++;
  while (strtok(NULL, " ") != NULL) {
    c->len++;
  } 
  c->args = malloc(sizeof(void*) * c->len+1);
  if (c->args == NULL) {perror("malloc failed\n");exit(errno);}
  //malloc space for each word and store in char **command
  int wordLen;
  char *ptr = line;
  for (int i = 0; i < c->len; i++) {
    wordLen = strlen(ptr);
    c->args[i] = malloc(sizeof(char) * wordLen+1);
    if (c->args[i] == NULL) {perror("malloc failed\n");exit(errno);}
    memcpy(c->args[i], ptr, wordLen+1);
    ptr += wordLen+1;
  }
  c->args[c->len] = NULL;
  return c;
}

struct command* getInput() {
  char *input = getLine();
  struct command *c = line2arr(input);
  free(input); input = NULL;
  return c;
}
