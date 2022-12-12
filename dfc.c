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

typedef struct Client {
    int dfs1Fd;
    int dfs2Fd;
    int dfs3Fd;
    int dfs4Fd;
} Client;

typedef struct addr_n_port {
    char* ip;
    int PORT;
} addr_n_port;

typedef struct Query {
    char* command;
    char* filename;
} Query;

typedef struct Pair {
    int first;
    int second;
} Pair;

addr_n_port get_addrnport(char* line);
void query_dfs();
void send_file(char* filename);
void recv_file(char* filename);
void handle_ls();
void init_queries(char** argv, int argc);
int is_up(int fd);

Client client;
Query queries[1000];
int query_size = 0;
int query_cur = 0;
int up[4];

Pair map[4][4];

int main(int argc, char** argv) {

    Pair p1;
    Pair p2;
    Pair p3;
    Pair p4;

    p1.first = 1;
    p1.second = 2;
    p2.first = 2;
    p2.second = 3;
    p3.first = 3;
    p3.second = 4;
    p4.first = 4;
    p4.second = 1;

    map[0][0] = p1;
    map[0][1] = p2;
    map[0][2] = p3;
    map[0][3] = p4;
    map[1][0] = p4;
    map[1][1] = p1;
    map[1][2] = p2;
    map[1][3] = p3;
    map[2][0] = p3;
    map[2][1] = p4;
    map[2][2] = p1;
    map[2][3] = p2;
    map[3][0] = p2;
    map[3][1] = p3;
    map[3][2] = p4;
    map[3][3] = p1;


    if(argc < 2){
        printf("Error: missing commands\n");
        return-1;
    }

    init_queries(argv, argc);
    
    FILE* file = fopen("dfc.conf", "rb");
    for(int i = 0; i < 4; i++) up[i] = 0;

    if (file != NULL) {
        ssize_t read;
        char* line = NULL;
        size_t len = 0;

        int count = 0;

        while((read = getline(&line, &len, file)) != -1){
            if(line[read-1] == '\n') line[read-1] = '\0';

            addr_n_port addr = get_addrnport(line);
            SA_IN their_addr;
            SA_IN sa;

            printf("ip: %s, PORT: %d\n", addr.ip, addr.PORT);

            inet_pton(AF_INET, addr.ip, &(sa.sin_addr));

            their_addr.sin_family = AF_INET;
            their_addr.sin_port = htons(addr.PORT);
            their_addr.sin_addr = sa.sin_addr;

            if(count == 0) {
                client.dfs1Fd = socket(AF_INET, SOCK_STREAM, 0);

                if(connect(client.dfs1Fd, (SA*)&their_addr, sizeof(SA)) == -1) {
                    printf("Error: connecting to dfs%d\n", count+1);
                }
                else up[count] = 1;
            }
            else if(count == 1) {
                client.dfs2Fd = socket(AF_INET, SOCK_STREAM, 0);

                if(connect(client.dfs2Fd, (SA*)&their_addr, sizeof(SA)) == -1) {
                    printf("Error: connecting to dfs%d\n", count+1);
                }
                else up[count] = 1;
            }
            else if(count == 2) {
                client.dfs3Fd = socket(AF_INET, SOCK_STREAM, 0);

                if(connect(client.dfs3Fd, (SA*)&their_addr, sizeof(SA)) == -1) {
                    printf("Error: connecting to dfs%d\n", count+1);
                }
                else up[count] = 1;
            }
            else if(count == 3) {
                client.dfs4Fd = socket(AF_INET, SOCK_STREAM, 0);

                if(connect(client.dfs4Fd, (SA*)&their_addr, sizeof(SA)) == -1) {
                    printf("Error: connecting to dfs%d\n", count+1);
                }
                else up[count] = 1;
            }

            count++;
        }

        for(int i = 0; i < 4; i++) printf("up[%d] is %d\n", i, up[i]);

        query_dfs(argv);

        close(client.dfs1Fd);
        close(client.dfs2Fd);
        close(client.dfs3Fd);
        close(client.dfs4Fd);
        fclose(file);
    }
    else {
        printf("Error: dfc.conf doesn't exist\n");
        return -1;
    }

    return 0;
}

addr_n_port get_addrnport(char* line) {
    addr_n_port out;
    out.ip = "";
    out.PORT = 0;
    char* Token = line;
    char* save = line;

    Token = strtok_r(Token, " \n", &save);
    Token = save;
    Token = strtok_r(Token, " \n", &save);
    Token = save;

    Token = strtok_r(Token, ":", &save);

    out.ip = Token;
    out.PORT = atoi(save);

    return out;
}

void query_dfs() {

    for(int i = 0; i < query_size; i++) {
        if(!strcmp(queries[i].command, "list")) {
            handle_ls();
        }
    }

    return;
}

void init_queries(char** argv, int argc) {
    int count = 0;

    for(int i = 1; i < argc; i++) {
        Query q;
        q.command = "";
        q.filename = "";

        q.command = argv[i];

        if(!strcmp(q.command, "list")){
            query_size++;
        }
        else if(!strcmp(q.command, "get")) {
            if(i+1 < argc) {
                q.filename = argv[i+1];
            }
            query_size++;
            i++;
        }
        else if(!strcmp(q.command, "put")) {
            if(i+1 < argc) {
                q.filename = argv[i+1];
            }
            query_size++;
            i++;
        }

        queries[count] = q;
        count++;
    }
}

void handle_ls() {
    int hash_set[256];
    int seen[256];
    char file_list[256][100];
    int fd_map[4];
    fd_map[0] = client.dfs1Fd;
    fd_map[1] = client.dfs2Fd;
    fd_map[2] = client.dfs3Fd;
    fd_map[3] = client.dfs4Fd;

    char sendBuffer[BUFF_SIZE];
    char recvBuffer[BUFF_SIZE]; 
    bzero(sendBuffer, BUFF_SIZE);
    bzero(recvBuffer, BUFF_SIZE);

    for(int i = 0; i < 256; i++) {
        hash_set[i] = 0;
        seen[i] = 0;

        for(int j = 0; j < 100; j++) file_list[i][j] = '\0';
    }

    for(int i = 0; i < 4; i++) {
        if(!is_up(fd_map[i])) {
            up[i] = 0;
        }
    }

    if(up[0]) {
        strcat(sendBuffer, "list");
        send(client.dfs1Fd, sendBuffer, strlen(sendBuffer), 0);
        recv(client.dfs1Fd, recvBuffer, BUFF_SIZE, 0);
    }
    else if(up[1]) {
        strcat(sendBuffer, "list");
        send(client.dfs2Fd, sendBuffer, strlen(sendBuffer), 0);
        recv(client.dfs2Fd, recvBuffer, BUFF_SIZE, 0);
    }
    else if(up[2]) {
        strcat(sendBuffer, "list");
        send(client.dfs3Fd, sendBuffer, strlen(sendBuffer), 0);
        recv(client.dfs3Fd, recvBuffer, BUFF_SIZE, 0);
    }
    else if(up[3]) {
        strcat(sendBuffer, "list");
        send(client.dfs4Fd, sendBuffer, strlen(sendBuffer), 0);
        recv(client.dfs4Fd, recvBuffer, BUFF_SIZE, 0);
    }

    char* Token = recvBuffer;
    char* save = recvBuffer;

    while(strcmp(save, "")) {
        Token = strtok_r(Token, " \n", &save);
        uint8_t *hash = md5String(Token);
        
        if(!seen[*hash]){
            strcpy(file_list[*hash], Token);
            seen[*hash] = 1;
        }

        Token = save;
    }

    for(int i = 0; i < 4; i++) {
        if(up[i]){
            for(int j = 0; j < 256; j++) {
                if(strcmp(file_list[j], "")) {
                    uint8_t* hash = md5String(file_list[j]);

                    Pair which;
                    int mod = *hash % 4;
                    which = map[mod][i];

                    hash_set[*hash] |= (1 << (which.first-1));
                    hash_set[*hash] |= (1 << (which.second-1));
                }
            }
        }
    }

    for(int i = 0; i < 256; i++) {
        if(strcmp(file_list[i], "")) {
            printf("hash_set[%d]: %d\n", i, hash_set[i]);
            if(hash_set[i] == 15) {
                printf("%s\n", file_list[i]);
            }
            else printf("%s is incomplete\n", file_list[i]);
        } 
    }

}

int is_up(int fd) {
    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);

    if(error != 0){
        return 0;
    }
    else return 1;
}