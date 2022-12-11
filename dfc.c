#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<time.h>
#include<errno.h>
#include<fcntl.h>

#include "md5.h"

#define BUFF_SIZE 15000
#define MAX_CLIENTS 30

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int main(int argc, char** argv) {
    printf("Hello world");
    
    return 0;
}