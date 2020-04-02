/* * A simple shell implementation
 * by Jaime Danguillecourt
 *
 * Handles I/O basic redirection (<, >, >>, 2>)
 * Only supports two builtins: cd & exit
 *
 * Behavior is undefined when using >> operator along with either > or 2>
 */

#include <stdio.h>  // printf
#include <stdlib.h> // getenv, malloc, free
#include <errno.h>  // errno
#include <string.h> // strtok, strcmp
#include <unistd.h> // fork, close, dup2, chdir
#include <wait.h>   // wait 
#include <fcntl.h>  // open
#include "prompt.h" // displayPrompt
#include "input.h"  // struct command, getInput

#define _POSIX_C_SOURCE 200809L

int ACTIVE = 1;

int myExit(char **args) {
  ACTIVE = 0;
  return 1;
}

int cd(char **args) {
  if (chdir(args[1]) == -1) {
    perror("cd failed");
  }
  return 1;
}

const char *BUILTINS[] = {
  "cd",
  "exit",
  NULL
};
const int (*BUILTINS_PTR[])(char **args) = {
  &cd,
  &myExit,
};
struct redirects {
  char *sOut;
  char *sIn;
  char *sErr;
  int append;
};


//find the filenames for redirection of stdin stdout stderr
struct redirects* parseArgs(struct command *c) {
  struct redirects *r = malloc(sizeof(struct redirects));
  if (r == NULL) {perror("malloc failed\n");exit(errno);}
  r->sOut = NULL;
  r->sIn = NULL;
  r->sErr = NULL;
  r->append = 0;

  for (int i = 0; i < c->len; i++) {
    if (strcmp(c->args[i], ">>") == 0) r->append = O_APPEND;
    c->args[i] = strtok(c->args[i], ">"); 
    if (c->args[i] == NULL) {
      r->sOut = c->args[i+1];
       strtok(NULL, ">"); 
      continue;
    }
    c->args[i] = strtok(c->args[i], "<"); 
    if (c->args[i] == NULL) {
      r->sIn = c->args[i+1];
      continue;
    }
    c->args[i] = strtok(c->args[i], "2>"); 
    if (c->args[i] == NULL) {
      r->sErr = c->args[i+1];
      continue;
    }
  }
  return r;
}

//set up file redirection for stdin, stdout, stderr
void handleRedirects(struct redirects *r) {
  int fd;
  if (r->sOut != NULL) {
    fd = open(r->sOut,O_WRONLY|O_CREAT|r->append,0640);
    if (fd == -1) {
      perror("file open failed\n");
      exit(errno);
    }
    dup2(fd, 1);
    close(fd);
  }
  if (r->sIn != NULL) {
    fd = open(r->sIn, O_RDONLY);
    if (fd == -1) {
      perror("file open failed\n");
      exit(errno);
    }
    dup2(fd, 0);
    close(fd);
  }
  if (r->sErr != NULL) {
    fd = open(r->sErr,O_WRONLY|O_CREAT,0640);
    if (fd == -1) {
      perror("file open failed\n");
      exit(errno);
    }
    dup2(fd, 2);
    close(fd);
  }
}

int runBuiltin(char **args) {
  for (int i = 0; BUILTINS[i] != NULL; i++) {
    if (strcmp(BUILTINS[i], args[0]) == 0) {
      (BUILTINS_PTR[i])(args);
      return 1;
    }
  }
  return 0;
}

void execute(char **args, struct redirects *r) {
    if (runBuiltin(args) == 1) {
      return;
    }

    if (fork() != 0) { // parent
      int *status;
      signal(SIGINT, SIG_IGN);
      signal(SIGKILL, SIG_IGN);
      wait(status);
    } else { // child
      handleRedirects(r);
      execvp(args[0], args);
      perror("exec failed");
      exit(errno);
    }
    signal(SIGINT, SIG_DFL);
    signal(SIGKILL, SIG_DFL);
}

void freeCommand(struct command *c) {
    for (int i = 0; c->args[i] != NULL; i++) {
      free(c->args[i]); c->args[i] = NULL;
    }
    free(c->args); c->args = NULL;
    free(c); c = NULL;
}

int main(int argc, char *argv[]) {
  char *prompt = getenv("PS1");
  while (ACTIVE == 1) {
    displayPrompt(prompt, errno);
    fflush(stdout);

    struct command *c = getInput();
    struct redirects *r = parseArgs(c);
    execute(c->args, r);
    free(r); r = NULL;
    freeCommand(c);
  }
  return 0;
}
