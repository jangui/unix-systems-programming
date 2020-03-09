/*
 * A simple shell implementation
 * by Jaime Danguillecourt
 *
 * Handles I/O basic redirection (<, >, >>, 2>)
 * Only supports two builtins: cd & exit
 */

#include <stdlib.h> //getenv
#include <errno.h>  //errno
#include "prompt.h"


int main(int argc, char *argv[]) {
  char *prompt = getenv("PS1");
  displayPrompt(prompt, errno);
  //for (;;) {
    //display prompt

 //parse input
 // -commands
 //   -builtin or custom
 // -io redirections

//run commands
//  fork, exec
//    get output from child
  return 0;
}
