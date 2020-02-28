#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/*
  TODO 
    tokenize files names
    if filename is curr dir, getSize of it, don't du it
*/

//struct used to keep track of hardlinks that have already been seen
struct hardlink {
  dev_t id;   //device number file exists on
  ino_t inum; //inode number of file
  int seen;   //count of how many times seen during traveral
};

/*
 struct stat {
    dev_t     st_dev;     ID of device containing file
    ino_t     st_ino;     inode number 
    mode_t    st_mode;    protection 
    nlink_t   st_nlink;   number of hard links
    uid_t     st_uid;     user ID of owner 
    gid_t     st_gid;     group ID of owner 
    dev_t     st_rdev;    device ID (if special file) 
    off_t     st_size;    total size, in bytes 
    blksize_t st_blksize; blocksize for file system I/O 
    blkcnt_t  st_blocks;  number of 512B blocks allocated 
    time_t    st_atime;   time of last access 
    time_t    st_mtime;   time of last modification 
    time_t    st_ctime;   time of last status change 
};
*/

#define DEBUG(x) printf("got here %d\n", x);

int getSize(char *filepath, int print_flag);

int du(char *folderpath) {
  DIR *d = opendir(folderpath);

  //try to open directory
  if (d == NULL) {
    fprintf(stderr, "%s: ", folderpath);
    perror("couldn't open directory");
    exit(errno);
  }

  //iterate over directory
  errno = 0;
  struct dirent *dir; ;
  int size = 0;
  while( (dir = readdir(d)) != NULL) {
    //get size of each item
    //don't of  parent dir
    if (strcmp(dir->d_name, "..") != 0) {
      size += getSize(dir->d_name, 0); 
    }
  }

  //close directory
  if (closedir(d) == -1) {
    fprintf(stderr, "%s: ", folderpath);
    perror("couldn't close directory");
    exit(errno);
  }
  
  //print total size of dir and return it
  printf("%d\t%s\n", size, folderpath);
  return size;
}

int getSize(char *filepath, int print_flag) {
  struct stat fs;
  //get file stats
  if (lstat(filepath, &fs) == -1) {
    fprintf(stderr, "%s: ", filepath);
    perror("couldn't get file stats"); 
    exit(errno);
  }
  
  //if directory and not link, recursive get size of directory
  //however if directory is curr directory, return size instead
  if (S_ISDIR(fs.st_mode) && !(S_ISLNK(fs.st_mode)) && strcmp(filepath, ".")!=0) {
    //change working directory to directory we are going to traverse
    if (chdir(filepath) == -1) {
      fprintf(stderr, "%s: ", filepath);
      perror("fail to change working directory");
      exit(errno);
    }
    //get size
    int size = du(filepath);
    
    //change working directory back
    if (chdir("..") == -1) {
      fprintf(stderr, "%s: ", filepath);
      perror("fail to change working directory");
      exit(errno);
    }
    return size;
  } 

  //print if flag set
  //only used if du is being used to calc one file's size
  if (print_flag == 1) printf("%ld\t%s\n", fs.st_blocks, filepath);
  return fs.st_blocks;
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
      getSize(argv[1], 1);
      break;
    default:
      //usage error
      fprintf(stderr, "usage: %s [optional] <filepath>\n", argv[0]);
      exit(1);
  }
  return 0;
}
