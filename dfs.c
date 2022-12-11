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

typedef struct Server {
	SA_IN server_addr;
	SA_IN client_addr;
	int serverFd;
    int PORT;
    char* dir;
	socklen_t len;
} Server;

typedef struct Request {
    char* command;
    char* filename;
} Request;

Server server;

void init_server(int PORT);
void kill_server();
void handle_client(int clientFd);
Request parse_command(char* source);
void handle_ls(int clientFd);
void recv_file(char* filename, int clientFd);
void update_metadata(char* filename);
void send_file(char* filename, int clientFd);

int main(int argc, char** argv) {
    if(argc < 3) {
        printf("Error: missing arguments\n");
        return -1;
    }

    server.dir = argv[1];
    int PORT = atoi(argv[2]);
    pid_t pid;

    signal(SIGINT, kill_server);
    init_server(PORT);

    while(1) {
        socklen_t client_len = sizeof(server.client_addr);
        int connectionFd = accept(server.serverFd, (SA*)&server.client_addr, &client_len);

        printf("Received connection...\n");

        if((pid = fork()) == 0) {
            printf("Handling requests of client with child process\n");

            close(server.serverFd);

            handle_client(connectionFd);

            printf("Connection with client got closed\n");

            close(connectionFd);
            exit(0);
        }

        close(connectionFd);   
    }

    return 0;
}

void kill_server() {
	close(server.serverFd);
	exit(0);
}

void init_server(int PORT) {
	if((server.serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error: failed to create server file descriptor\n");
		exit(-1);
	}

	memset(&server.server_addr, 0, sizeof(server.server_addr));
	server.server_addr.sin_family = AF_INET;
	server.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server.server_addr.sin_port = htons(PORT);
	server.len = sizeof(server.server_addr);

	if(bind(server.serverFd, (SA*)&server.server_addr, server.len) < 0) {
		printf("Error: failed to bind to PORT NUMBER\n");
		exit(-1);
	}

	listen(server.serverFd, MAX_CLIENTS);
	printf("Server ready to accept connections\n");
}

void handle_client(int clientFd) {

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    int bytes_received = 0;

    char recvBuffer[BUFF_SIZE];
    char sendBuffer[BUFF_SIZE];
    bzero(recvBuffer, BUFF_SIZE);
    bzero(sendBuffer, BUFF_SIZE);

    while((bytes_received = recv(clientFd, recvBuffer, BUFF_SIZE, 0)) > 0) {

        Request req = parse_command(recvBuffer);

        printf("Req command: %s\n", req.command);
        printf("Req filename: %s\n", req.filename);

        if(!strcmp(req.command, "list")) {
            handle_ls(clientFd);
        }
        else if(!strcmp(req.command, "get")) {
            send_file(req.filename, clientFd);
        }
        else if(!strcmp(req.command, "put")) {
            recv_file(req.filename, clientFd);
        }

        bzero(recvBuffer, BUFF_SIZE);
    }

    return;
}

Request parse_command(char* source) {
    Request out;

    char* token = source;
    char* save = "";

    token = strtok_r(token, " ", &save);

    out.command = token;
    out.filename = save;

    return out;
}

void handle_ls(int clientFd) {
    return;
}

void recv_file(char* filename, int clientFd) {
    return;
}

void send_file(char* filename, int clientFd) {
    return;
}