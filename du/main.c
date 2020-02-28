#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BLOCK_SIZE 512

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

/*
int du(char *folderpath) {
  dir = opendir(folderpath);
  struct dirent *dir = readdir(d);
  //iterate over directory and call getsize
  int check = closedir(dir);
  return;
}
*/

int getSize(char *filepath, int print_flag) {
  struct stat fs;
  if (lstat(filepath, &fs) == -1) {
    perror("couldn't get file stats"); 
    exit(errno);
  }
  if (S_ISDIR(fs.st_mode) && !(S_ISLNK(fs.st_mode))) {
    return 0;
    //return du();
  } 
  //TODO fix block size calcs to not be hardcoded
  int size = fs.st_blocks / 2;
  if (print_flag == 1) printf("%d\t%s\n", size, filepath);
  return size;
}

int main(int argc, char *argv[]) {
  DIR *dir;
  switch (argc) {
    case 1:
      //default behavior
      //du(".");
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
