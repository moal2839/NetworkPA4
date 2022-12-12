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

typedef struct file_data {
    int uid;
    long chunk_size;
    int chunk_num;
} file_data;

Server server;

void init_server(int PORT);
void kill_server();
void handle_client(int clientFd);
Request parse_command(char* source);
void handle_ls(int clientFd);
void handle_put(char* filename, int clientFd);
void recv_file(int file, int clientFd, long size);
void update_metadata(char* filename);
void send_file(char* filename, int clientFd);
file_data parse_header(char* source);

int main(int argc, char** argv) {
    if(argc < 3) {
        printf("Error: missing arguments\n");
        return -1;
    }

    char pathname[100];
    bzero(pathname, 100);

    strcat(pathname, argv[1]);
    strcat(pathname, "/metadata-logfile.txt");

    printf("pathname: %s\n", pathname);

    if(access(pathname, F_OK) != 0) {
        FILE* f = fopen(pathname, "wb");
        fclose(f);
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

    // struct timeval timeout;
    // timeout.tv_sec = 10;
    // timeout.tv_usec = 0;

    // setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
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
            char* msg = "ack";

            send(clientFd, msg, strlen(msg), 0);

            handle_put(req.filename, clientFd);
        }

        bzero(recvBuffer, BUFF_SIZE);
    }

    return;
}

Request parse_command(char* source) {
    Request out;

    char* token = source;
    char* save = "";

    token = strtok_r(token, " \n", &save);

    out.command = token;
    out.filename = save;

    return out;
}

void handle_ls(int clientFd) {
    char pathname[100];
    bzero(pathname, 100);    

    strcat(pathname, server.dir);
    strcat(pathname, "/metadata-logfile.txt");

    FILE* f = fopen(pathname, "rb");

    ssize_t read;
    char* line = NULL;
    size_t len = 0;
    int bytes = 0;

    char sendBuffer[BUFF_SIZE];
    bzero(sendBuffer, BUFF_SIZE);

    while((read = getline(&line, &len, f)) != -1) {
        if(line[read-1] == '\n') line[read-1] = '\0';

        char* Token = line;
        char* save;
        Token = strtok_r(Token, " \n", &save);

        printf("Filename: %s\n", Token);

        strcat(sendBuffer, Token);
        strcat(sendBuffer, "\n");

        bytes += strlen(Token);
        bytes += 1;
    }

    send(clientFd, sendBuffer, bytes, 0);

    return;
}

void handle_put(char* filename, int clientFd) {
    char recvBuffer[BUFF_SIZE];
    bzero(recvBuffer, BUFF_SIZE);

    char file_type[15];
    bzero(file_type, 15);

    if(strstr(filename, ".")) {
        int count = 0;
        int start = 0;
        int n = strlen(filename);

        for(int i = 0; i < n; i++) {
            if(filename[i] == '.' && !start) {
                start = 1;
                file_type[count] = filename[i];
                count++;
            }
            else if(start) {
                file_type[count] = filename[i];
                count++;
            }
        }
    }

    recv(clientFd, recvBuffer, BUFF_SIZE, 0);

    file_data f_data = parse_header(recvBuffer);

    char* msg = "ack";

    send(clientFd, msg, strlen(msg), 0);

    bzero(recvBuffer, BUFF_SIZE);

    char pathname[100];
    bzero(pathname, 100);
    strcat(pathname, server.dir);
    strcat(pathname, "/");

    char id[3];
    bzero(id, 3);

    sprintf(id, "%d", f_data.uid);

    char hash_with_id[100];
    bzero(hash_with_id, 100);
    strcat(hash_with_id, filename);
    strcat(hash_with_id, id);

    uint8_t* p = md5String(hash_with_id);

    char digest[33];
    bzero(digest, 33);

    sprintf(digest, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", p[0], 
    p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13],
    p[14], p[15]);

    strcat(pathname, digest);
    
    char num[2];
    bzero(num, 2);

    sprintf(num, "%d", f_data.chunk_num);

    char pathname2[100];
    bzero(pathname2, 100);

    strcat(pathname2, pathname);

    strcat(pathname, "-");
    strcat(pathname, num);

    if(strcmp(file_type, "")){
        strcat(pathname, file_type);
    }

    printf("pathname: %s\n", pathname);

    FILE* file = fopen(pathname, "wb");

    recv_file(fileno(file), clientFd, f_data.chunk_size);

    printf("here\n");

    fclose(file);
    recv(clientFd, recvBuffer, BUFF_SIZE, 0);

    file_data f_data2 = parse_header(recvBuffer);

    send(clientFd, msg, strlen(msg), 0);

    strcat(pathname2, "-");
    bzero(num, 2);
    sprintf(num, "%d", f_data2.chunk_num);
    strcat(pathname2, num);

    if(strcmp(file_type, "")){
        strcat(pathname2, file_type);
    }

    printf("pathname2: %s\n", pathname2);

    FILE* file2 = fopen(pathname2, "wb");

    recv_file(fileno(file2), clientFd, f_data2.chunk_size);

    fclose(file2);
}

void recv_file(int file, int clientFd, long size) {
    char recvBuffer[BUFF_SIZE];
    bzero(recvBuffer, BUFF_SIZE);

    long bytes = 0;
    long bytesRecv = 0;

    while( bytes < size && (bytesRecv = recv(clientFd, recvBuffer, BUFF_SIZE, 0))) {
        bytes += bytesRecv;
        printf("bytes: %ld\n", bytes);

        write(file, recvBuffer, bytesRecv);

        bzero(recvBuffer, BUFF_SIZE);
    }

    char* msg = "received";

    send(clientFd, msg, strlen(msg), 0);    
}

void send_file(char* filename, int clientFd) {
    return;
}

file_data parse_header(char* source) {
    file_data out;
    out.chunk_num = 0;
    out.chunk_size = 0;
    out.uid = 0;

    char* Token = source;
    char* save = source;

    Token = strtok_r(Token, " \n", &save);
    Token = save;
    Token = strtok_r(Token, " \n", &save);
    out.uid = atoi(Token);
    Token = save;
    Token = strtok_r(Token, " \n", &save);
    Token = save;
    Token = strtok_r(Token, " \n", &save);
    out.chunk_num = atoi(Token);
    Token = save;
    Token = strtok_r(Token, " \n", &save);
    out.chunk_size = atol(save);

    printf("uid: %d, chunk-num: %d, chunk-size: %ld\n", out.uid, out.chunk_num, out.chunk_size);

    return out;
}