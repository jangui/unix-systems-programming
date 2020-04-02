#include <stdlib.h>         // exit
#include <stdio.h>          // printf, fputs, fprintf, stderr
#include <unistd.h>         // read
#include <string.h>         // memset
#include <sys/socket.h>     // socket, AF_INET, SOCK_STREAM
#include <arpa/inet.h>      // inet_pton
#include <netinet/in.h>     // servaddr
#include <errno.h>          // errno


#define	MAXLINE 4096	// max text line length 
#define	BUFFSIZE 8192	// buffer size for reads and writes 
#define PORT 1337

int main(int argc, char** argv) {
  //usage check
  if (argc != 2) {
    fprintf(stderr, "usage: %s <IPaddress>\n", argv[0]);
    exit(errno);
  }

  //make socket
  int sockfd;  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "socket error");
      exit(errno);
  }

  //setup for connect
  struct sockaddr_in	servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(PORT);
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
      fprintf(stderr, "inet_pton error for: %s\n", argv[1]);
      exit(errno);
  }

  //connect
  if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
    exit(errno);
  }

  //readline
  int n;
  char recvline[MAXLINE + 1];
  while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
    recvline[n] = '\0';
    if (fputs(recvline, stdout) == EOF) {
      fprintf(stderr,"fputs Error\n");
      exit(errno);
    }
  }
  
  //check if read was successful
  if (n < 0) {
      fprintf(stderr, "read error\n");
      exit(errno);
  }
  close(sockfd);

  return 0;
}
