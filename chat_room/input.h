#define MAXLINE   4096

char *recvLine(int fd, int maxline);
char *getLine(int maxline);
int sendLine(int fd, char *msg);
char *getName(char *msg);
char *addNewLine(char *msg);
