#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 64

enum operation {
    INIT,
    REQUEST,
    LS_PENDING,
    RUN
};

int init();
int request();
int ls_pending();
int run();

int main(int argc, char** argv)
{
    enum operation operation;
    char commend[BUF_SIZE];

    if(argc > 2){
        printf("Too many arguments.\n");
        return 1;
    }
    //parse option

    switch(operation)
    {
        case INIT:
        return init();
        case REQUEST:
        return request(commend);
        case LS_PENDING:
        return ls_pending();
        case RUN:
        return run();
        default:
        printf("Something goes wrong.\n");
        return 1;
    }
}

int init()
{
    char ip_addr[16];
    int port_num;
    FILE *fptr;

    if(getuid() != geteuid()){
        printf("Permission denied.\n");
        printf("Run commend as root.\n");
        return -1;
    }

    printf("Enter the IP Address >> ");
    scanf("%s", ip_addr);
    printf("Enter the Port number >> ");
    scanf("%d", port_num);

    fptr = fopen("apsudo.config", "w");
    fprintf(fptr, "%s:%d", ip_addr, port_num);
    fclose(fptr);
    
    return 0;
}

int request(char* commend)
{
    int sock;
    ssize_t len;
    char token[BUF_SIZE];
    char ip_addr[16];
    char temp[32];
    char* tok;
    int port_num;
    struct sockadd_in server_addr;
    FILE *fptr;

    if(access("apsudo.config", F_OK) == -1){
        printf("Run \'apsudo --init\' first.\n");
        return -1;
    }

    fptr = fopen("apsudo.config", "r");
    fgets(temp, sizeof(temp), fptr);
    tok = strtok("temp", ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' commend.\n");
        return -1;
    }
    strcpy(ip_addr, tok);
    tok = strtok(NULL, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' commend.\n");
        return -1;
    }
    port_num = atoi(tok);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("Fail to create socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_port = htons(port_num);
    
    if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Can't connect to server");
        return -1;
    }

    len = write(sock, commend, sizeof(commend));
    if(len < 0)
    {
        perror("Fail to send commend");
        return -1;
    }

    len = read(sock, token, sizeof(token));
    if(len < 0)
    {
        perror("Fail to receive token");
        return -1;
    }
    
    close(sock);

    if(strncmp(token, "Approve", (size_t)len) == 0){
        printf("Approved!\n");
        printf("Run commend: \'%s\'\n", commend);
        system(commend);
    }
    else if(strcmp(token, "Deny", (size_t)len) == 0){
        printf("Denied!\n");
        printf("Contect to administrater.\n");
    }
    else if(strcmp(token, "Pending", (size_t)len) == 0){
        printf("Pending!\n");
        printf("Run apsudo --ls-pending to see approved commend.\n");
    }
    else{
        printf("Token is broken.");
        return -1;
    }

    return 0;
}