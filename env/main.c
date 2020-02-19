#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define ALLOC_CHECK(ptr) if ((ptr) == NULL) {perror("mem alloc failed");exit(errno);}

extern char **environ;

//displays every elem in environment
void print_env(char **env) {
  while (env[0] != NULL) {
    printf("%s\n", env[0]);
    env++;
  } 
}

//returns count of consecutive key val pairs from start of char**
int count_key_vals(char *v[]) {
  int i;
  if (v == NULL) return -1;
  for (i=0; v[i] != NULL; i++) {
    if (strchr(v[i], '=') == NULL) return i;
  }
  return i;
}

//counts number of consecutives key vals pairs from start of v and
//returns a char** with space to put them in
char **malloc_new_env(char *v[]) {
  int count = count_key_vals(v);
  char **env = calloc(count+1, sizeof(char*));
  ALLOC_CHECK(env)
  env[count] = NULL;
  return env;
}

//checks if a string in the format KEY=VALUE has an existing KEY match in v
//returns 1 if found, 0 else
int keyInV(char *s, char *v[]) {
  if (v == NULL) return 0;
  char *sCpy = malloc(sizeof(char)*(strlen(s)+1));
  ALLOC_CHECK(sCpy) 
  strcpy(sCpy, s);
  while(v[0] != NULL) {
    char *vsCpy = malloc(sizeof(char)*((strlen(v[0])+1)));
    ALLOC_CHECK(vsCpy)
    strcpy(vsCpy, v[0]);
    //char *vKey = strtok(vsCpy, "=");
    vsCpy = strtok(vsCpy, "=");
    sCpy = strtok(sCpy, "=");
    if (strcmp(sCpy, vsCpy) == 0){
      return 1;
    }
    v++;
  }
  return 0;
}

//returns size of char**
int sizeOfV(char *v[]) {
  if (v == NULL) return 0;
  int c = 0;
  while (v[0] != NULL) {
    c++; 
    v++;
  }
  return c;
}


char **move_env_to_heap() {
  int size = 0;
  char **ptr = environ;
  while(ptr[0] != NULL) {
    size++; 
    ptr++;
  }
  char **new_env = malloc(sizeof(char*)*(size+1));
  ALLOC_CHECK(new_env)

  for (int i=0; environ[i] != NULL; i++) {
    new_env[i] = malloc(strlen(environ[i])+1);
    ALLOC_CHECK(new_env[i])
    strcpy(new_env[i], environ[i]);
  }
  return new_env;
}

//inserts s into v
//v must be in heap
char **insert_key_val(char *s, char *v[]) {
//  print_env(v);
  int size = sizeOfV(v) + 1;    
  v = realloc(v, sizeof(char*)*(size+1));
  ALLOC_CHECK(v)
  v[size-1] = realloc(v[size-1], sizeof(char)*(strlen(s)+1));
  ALLOC_CHECK(v[size-1])
  strcpy(v[size-1], s);
  v[size] = NULL;
  return v;
  
}

//find key in v of key=value
int find_key(char* key, char *v[]) {
  for (int i=0; v[i] != NULL; i++) {
    char *vsCpy = malloc(sizeof(char*)*(strlen(v[i])+1));
    ALLOC_CHECK(vsCpy)
    strcpy(vsCpy, v[i]);
    char *vskey = strtok(vsCpy, "=");
    if (strcmp(vskey, key) == 0) {
      return i;
    }
  }
  return -1;
}
   
char **modify_key_val(char *str, char *v[]) {
  //seperate string into key and val
  char *s = malloc(sizeof(char)*(strlen(str)+1)); 
  ALLOC_CHECK(s)
  strcpy(s, str);
  char *skey = strtok(s, "=");
  char *sval = strtok(NULL, "=");
  //find key in v
  int keyInd = find_key(skey, v);
  if (keyInd == -1) return NULL;
  //realloc size of key to fit new val
  v[keyInd] = realloc(v[keyInd], sizeof(char)*(strlen(v[keyInd])+1+strlen(sval)+1)); 
  ALLOC_CHECK(v[keyInd])
  //alter key to have new val
  char *vKey = strtok(v[keyInd], "=");
  strcat(v[keyInd], "=");
  strcat(v[keyInd], sval);
  return v;
}

//returns a char** after adding or modifying key values pairs to environ from v
char **alter_environ(char *v[]) {
  if (v[0] == NULL) return environ;

  while (v[0] != NULL && strchr(v[0], '=') != NULL) {
    if (keyInV(v[0], environ) == 1) {
      environ = modify_key_val(v[0], environ);
    } else {
      environ = insert_key_val(v[0], environ);
    }
    v++;
  }
  return environ;
}

//parse argv of keyvals until start of command found and execv on it
int run_command(char *argv[], char **env) {
  int ind = count_key_vals(argv);
  if (argv[ind] == NULL) {
    print_env(env); 
    return 0;
  } else {
    execvp(argv[ind], argv + ind);
    //only get here if exec failed
    return 1;
  }
  
}

void free_env(char **env) {
  for (int i=0; env[i] != NULL; i++) {
    free(env[i]); 
    env[i] = NULL;
  }
  free(env);
}

int main(int argc, char *argv[]) {
  int offset = 0;
  int flag = 0;
  if (argc == 1) {
    print_env(environ); 
    return 0;
  } else if (strchr(argv[1], '-') != NULL) {
    //-i is set 
    offset = 2;
    environ = malloc_new_env(argv+offset);
  } else {
    //-i is not set 
    offset = 1;
    //moving environ to heap to take advantage of realloc
    //waste of space but makes code nicer
    environ = move_env_to_heap(); 
  }
  environ = alter_environ(argv+offset);
  int exec = run_command(argv+offset, environ);
  //we only get here if exec failed or no command was passed
  //free up env
  free_env(environ);
  environ = NULL;

  if (exec == 1) {
    perror("exec failed"); 
    exit(errno);
  }
  return 0;
}
