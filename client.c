#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

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
    char command[BUF_SIZE];
    int c, i;
    
    static struct option long_options[] =
    {
        {"init", no_argument, 0, 'i'},
        {"ls-pending", no_argument, 0, 'l'},
        {"run", no_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    int option_index = 0;

    c = getopt_long(argc, argv, "", long_options, &option_index);

    switch(c)
    {
        case 'i':
        operation = INIT; break;
        case 'l':
        operation = LS_PENDING; break;
        case 'r':
        operation = RUN; break;
        case '?':
        return -1;
        default:
        operation = REQUEST;
    }

    switch(operation)
    {
        case INIT:
        return init();
        case REQUEST:
        if(argc < 2){
            printf("Enter your command.\n");
            return 0;
        }
        strcpy(command, argv[1]);
        for(i=2; i<argc; i++){
            strcat(command, " ");
            strcat(command, argv[i]);
        }
        printf("%s\n", command);
        return request(command);
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
        printf("Run command as root.\n");
        return -1;
    }

    printf("Enter the IP Address >> ");
    scanf("%s", ip_addr);
    printf("Enter the Port number >> ");
    scanf("%d", &port_num);

    fptr = fopen("apsudo.config", "w");
    fprintf(fptr, "%s:%d", ip_addr, port_num);
    fclose(fptr);

    return 0;
}

int request(char* command)
{
    int sock;
    ssize_t len;
    char token[BUF_SIZE];
    char ip_addr[16];
    char temp[32];
    char quary[64];
    char* tok;
    int port_num;
    struct sockaddr_in server_addr;
    FILE *fptr;

    if(access("apsudo.config", F_OK) == -1){
        printf("Run \'apsudo --init\' first.\n");
        return -1;
    }

    fptr = fopen("apsudo.config", "r");
    fgets(temp, sizeof(temp), fptr);
    tok = strtok(temp, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
        return -1;
    }
    strcpy(ip_addr, tok);
    tok = strtok(NULL, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
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

    strcpy(quary, "request:");
    strcat(quary, command);

    len = write(sock, quary, strnlen(quary, sizeof(quary)));
    if(len < 0)
    {
        perror("Fail to send command");
        return -1;
    }

    len = read(sock, token, sizeof(token));
    if(len < 0)
    {
        perror("Fail to receive token");
        return -1;
    }
    
    close(sock);

    if(strncmp(token, "Approve", sizeof(token)) == 0){
        printf("Approved!\n");
        printf("Run command: \'%s\'\n", command);
        system(command);
    }
    else if(strncmp(token, "Deny", sizeof(token)) == 0){
        printf("Denied!\n");
        printf("Contect to administrater.\n");
    }
    else if(strncmp(token, "Pend", sizeof(token)) == 0){
        printf("Pending!\n");
        printf("Run apsudo --ls-pending to see approved command.\n");
    }
    else{
        printf("token: %s\n", token);
        printf("Token is broken.\n");
        return -1;
    }

    return 0;
}

int ls_pending()
{
    const char *quary;
    int sock;
    ssize_t len;
    char token[BUF_SIZE];
    char ip_addr[16];
    char temp[32];
    char* tok;
    int port_num;
    struct sockaddr_in server_addr;
    FILE *fptr;

    if(access("apsudo.config", F_OK) == -1){
        printf("Run \'apsudo --init\' first.\n");
        return -1;
    }

    fptr = fopen("apsudo.config", "r");
    fgets(temp, sizeof(temp), fptr);
    tok = strtok(temp, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
        return -1;
    }
    strcpy(ip_addr, tok);
    tok = strtok(NULL, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
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

    quary = "ls_pending:approve";
    len = write(sock, quary, strlen(quary) + 1);
    if(len < 0)
    {
        perror("Fail to send command");
        return -1;
    }

    printf("Approved: \n");
    while((len = read(sock, token, sizeof(token))) > 0)
    {
        printf("%s", token);
    }

    quary = "ls_pending:pending";
    len = write(sock, quary, strlen(quary) + 1);
    if(len < 0)
    {
        perror("Fail to send command");
        return -1;
    }

    printf("Pending: \n");
    while((len = read(sock, token, sizeof(token))) > 0)
    {
        printf("%s", token);
    }
    
    close(sock);

    return 0;
}

int run()
{
    const char *quary = "run:";
    int sock;
    ssize_t len;
    char token[BUF_SIZE];
    char ip_addr[16];
    char temp[32];
    char* tok;
    int port_num;
    struct sockaddr_in server_addr;
    FILE *fptr;

    if(access("apsudo.config", F_OK) == -1){
        printf("Run \'apsudo --init\' first.\n");
        return -1;
    }

    fptr = fopen("apsudo.config", "r");
    fgets(temp, sizeof(temp), fptr);
    tok = strtok(temp, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
        return -1;
    }
    strcpy(ip_addr, tok);
    tok = strtok(NULL, ":");
    if(!tok){
        printf("apsudo.config is broken.\n");
        printf("To recover run \'apsudo --init\' command.\n");
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

    len = write(sock, quary, strlen(quary) + 1);
    if(len < 0)
    {
        perror("Fail to send command");
        return -1;
    }

    printf("Approved: \n");
    while((len = read(sock, token, sizeof(token))) > 0)
    {
        printf("%s", token);
    }

    return 0;
}