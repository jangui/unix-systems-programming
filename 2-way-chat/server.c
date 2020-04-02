#include <unistd.h>       //write, close
#include <stdio.h>        //fprint, stderr
#include <stdlib.h>       //exit
#include <string.h>       //memset, strlen
#include <errno.h>        //errno
#include <netinet/in.h>   //servaddr, INADDR_ANY, htons, htonl
#include <sys/socket.h>   //socket, AF_INET, SOCK_STREAM
                          //bind, liste, accept

#define PORT_NUM 1337
#define LISTENQ 1024

//socket, bind, listen, accept
int main(int argc, char** argv) {
  //usage check
  if (argc != 3) {
  
  }

  //create socket, ipv4 using tcp
  int socketfd;
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error");
    exit(errno);
  }
  
  //setup sockaddr_in
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT_NUM);
  //bind
  bind(socketfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  //listen
  listen(socketfd, LISTENQ);
  
  int connfd;
  //process each connection
  for (;;) { 
    //accept incoming connection
    connfd = accept(socketfd, NULL, NULL);
    char* buf = "hello there\n";
    write(connfd, buf, strlen(buf));
    close(connfd);
  }

  return 0;
}
