#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

//default capacity for vector holding seen inodes
#define DEFAULT_CAPACITY 10

//LL of seen inodes
struct inoLL{
  dev_t id;   //device number file exists on
  ino_t inum; //inode number of file
  struct inoLL *next;
} typedef inoLL;

inoLL* insertLL(inoLL *head, dev_t id, ino_t inum) {
  struct inoLL *ll = malloc(sizeof(inoLL));
  if (ll == NULL) {perror("malloc failed");exit(errno);}
  ll->id = id;
  ll->inum = inum;
  ll->next = head;
  return ll;
}

//returns 1 if inode in LL else -1
int inoInLL(dev_t id, ino_t inum, inoLL *head) {
  inoLL *ptr = head;
  while (ptr != NULL) {
    if ( (ptr->id == id) && (ptr->inum == inum)) {
      return 1;
    } 
    ptr = ptr->next;
  }
  return -1;
}

void freeLL(inoLL *head) {
  inoLL *ptr = head;
  while (head != NULL) {
    head = head->next;
    free(ptr);
    ptr = head;
  } 
}


blkcnt_t getSize(char *filename) {
  //get file stats
  struct stat fs;
  if (lstat(filename, &fs) == -1) {
    fprintf(stderr, "%s: ", filename);
    perror("couldn't get file stats"); 
    exit(errno);
  }
  return fs.st_blocks;
}

//concatinates 2 strings
//make sure to free later
char *concat(char *s1, char *s2) {
  char *newName = malloc(sizeof(char) * (strlen(s1)+1+strlen(s2)+1));
  if (newName == NULL) {perror("malloc failed");exit(errno);}
  strcpy(newName, s1);
  strcat(newName, s2);
  return newName;
}


blkcnt_t handleFile(char *folderName, DIR *d, inoLL *head)  {

  struct dirent *dir;
  blkcnt_t size = 0;

  //iterate over directory
  errno = 0;
  while( (dir = readdir(d)) != NULL) {
    if (errno != 0) {perror("readdir failed");exit(errno);}
    char *fp = concat(folderName, dir->d_name);

    struct stat fs;
    //get file stats
    if (lstat(fp, &fs) == -1) {
      fprintf(stderr, "%s: ", fp);
      perror("couldn't get file stats"); 
      exit(errno);
    }

    //if dir and not .. or ., recurse
    if (S_ISDIR(fs.st_mode) && !(S_ISLNK(fs.st_mode)) && strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {

      blkcnt_t dir_size = getSizeDir(fp, head);
      size += dir_size;
      printf("%ld\t%s\n", dir_size, fp);
      head = insertLL(head, fs.st_dev, fs.st_ino);

    } else if (strcmp(dir->d_name, "..") != 0) {
      int flag = inoInLL(fs.st_dev, fs.st_ino, head);
      if (flag == -1) {
        size += getSize(fp);
        head = insertLL(head, fs.st_dev, fs.st_ino);
      }
    }
    free(fp); fp == NULL;
  }
}

blkcnt_t getSizeDir(char *folderpath, inoLL *head) {
  //try to open directory
  DIR *d = opendir(folderpath);
  if (d == NULL) {
    fprintf(stderr, "%s: ", folderpath);
    perror("couldn't open directory");
    exit(errno);
  }
  char *folderName = concat(folderpath, "/");
  handleFile(folderpath, d, head);
  //close directory
  if (closedir(d) == -1) {
    fprintf(stderr, "%s: ", folderpath);
    perror("couldn't close directory");
    exit(errno);
  }
  free(folderName); folderName == NULL;

  return size;
}

void du(char *filepath) {
  struct stat fs;

  //get file stats
  if (lstat(filepath, &fs) == -1) {
    fprintf(stderr, "%s: ", filepath);
    perror("couldn't get file stats"); 
    exit(errno);
  }

  //if directory and not symlink, recurse on directory
  if (S_ISDIR(fs.st_mode) && !(S_ISLNK(fs.st_mode))) {
    inoLL *head = malloc(sizeof(inoLL));
    head->id = -1;
    head->inum = -1;
    head->next = NULL;
    blkcnt_t size = getSizeDir(filepath, head);
    printf("%ld\t%s\n", size, filepath);
    freeLL(head);

  } else if (S_ISREG(fs.st_mode)) {
    printf("%ld\t%s\n", fs.st_blocks, filepath);
  }
}

int main(int argc, char *argv[]) {
  DIR *dir;
  switch (argc) {
    case 1:
      //default behavior
      du(".");
      break;
    case 2: ;
      //filepath provided
      du(argv[1]);
      break;
    default:;
      //usage error
      fprintf(stderr, "usage: %s [optional] <filepath>\n", argv[0]);
      exit(1);
  }
  return 0;
}
