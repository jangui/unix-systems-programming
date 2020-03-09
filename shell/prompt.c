#include <limits.h> //PATH_MAX
#include <unistd.h> //getcwd, gethostname, getuid
#include <pwd.h> //getpwuid, struct passwd 
#include <errno.h> //errno
#include <stdio.h> //printf, perror
#include <stdlib.h> //exit
#include <string.h> //strcmp

char *getUserName() {
  uid_t uid = geteuid();
  struct passwd *pw = getpwuid(uid);
  if (pw) return pw->pw_name;
  return "";
}

void displayPrompt(char *prompt, int err) {
  //get cwd
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
       perror("getcwd() error");
       exit(errno);
  }
  //get username
  char *username = getUserName();
  //get hostname
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == -1) {
    perror("error getting host name");
    exit(errno);
  }
  //display prompt
  if (prompt != NULL) {
    printf("%s", prompt); 
  } else {
    printf("\033[1;34m"); //set color to blue
    printf("┌─["); 
    printf("\033[1;35m"); //set color to magenta
    printf("%s", username); 
    printf("\033[1;32m"); //set color to green
    printf("@");
    printf("\033[1;35m"); //set color to magenta
    printf("%s", hostname);
    printf("\033[1;34m"); //set color to blue
    printf("]");
    printf("\033[0m"); //reset normal color
    printf(" - "); 
    printf("\033[1;34m"); //set color to blue
    printf("[");
    printf("\033[0m"); //reset normal color
    printf("%s", cwd);
    printf("\033[1;34m"); //set color to blue
    printf("]\n└─[");
    printf("\033[1;32m"); //set color to green
    printf("%d", err);
    printf("\033[1;34m"); //set color to blue
    printf("]");
    printf("\033[1;31m"); //set color to red
    printf(" $ ");
    printf("\033[0m"); //reset normal color
  }
}
