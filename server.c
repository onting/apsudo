#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gmodule.h>

#define BUF_SIZE 32
#define DEFAULT_PORTNUM 2003

gint CompareNames (gconstpointer name1, gconstpointer name2)
{
	return strcmp((const char*)name1, (const char*)name2);
}

void init(GTree* ruleTree);

int main()
{
    pid_t pid;

    pid = fork();

    if(pid < 0)
    {
        perror("Daemon Create Fail");
        exit(EXIT_FAILURE);
    }

    if(pid > 0)
        exit(EXIT_SUCCESS);

    /*child*/
    if(setsid() < 0)
        exit(EXIT_FAILURE);

    char buffer[BUF_SIZE];
    struct sockaddr_in server_addr, client_addr;
    char temp[32];
    int server_fd, client_fd;
    int port_num;
    socklen_t client_len;
    int len;
    char operation[16];
    char operation_arg[16];
    char *tok;
    FILE* fptr;
    GTree* ruleTree = g_tree_new(CompareNames);
    GQueue* q_approve = g_queue_new();
    GQueue* q_pending = g_queue_new();
    int i;
    char *sptr;

    init(ruleTree);

    if(access("portnum.config", F_OK) == -1)
        port_num = DEFAULT_PORTNUM;
    else{
        fptr = fopen("portnum.config", "r");
        fgets(temp, sizeof(temp), fptr);
        port_num = atoi(temp);
        fclose(fptr);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
        exit(EXIT_FAILURE);

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_num);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        exit(EXIT_FAILURE);

    if(listen(server_fd, 5) < 0)
        exit(EXIT_FAILURE);

    client_len = sizeof(client_addr);
    while(1)
    {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if(client_fd < 0)
            exit(EXIT_FAILURE);
        
        len = read(client_fd, buffer, sizeof(buffer));
        if(len < 0)
            exit(EXIT_FAILURE);
        
        tok = strtok(buffer, ":");
        strncpy(operation, tok, sizeof(operation));
        tok = strtok(NULL, ":");
        strncpy(operation_arg, ":", sizeof(operation_arg));

        if(strncmp(operation, "request", sizeof(operation)) == 0)
        {
            for(i=0; operation_arg[i] && (operation_arg[i] != ' '); i++){
                temp[i] = operation_arg[i];
            }
            temp[i] = 0;
            sptr = g_tree_search(ruleTree, CompareNames, (gconstpointer)temp);
            if(sptr){ //program rule is found
                strncpy(buffer, sptr, sizeof(buffer));
                if(strncmp(buffer, "Pend", sizeof(buffer)) == 0){
                    g_queue_push_tail(q_pending, operation_arg); //push command to pending queue
                }
            }
            else{ //program rule is not found
                strncpy(buffer, "Deny", sizeof(buffer));
            }
            write(client_fd, buffer, strnlen(buffer, sizeof(buffer)));
        }
        else if(strncmp(operation, "ls_pending", sizeof(operation)) == 0)
        {
            if(strncmp(operation_arg, "approve", sizeof(operation_arg)) == 0)
            {
                if(g_queue_is_empty(q_approve)){
                    write(client_fd, "", 0);
                }
                else{
                    for(i = 0; sptr = g_queue_peek_nth(q_approve, i); i++){
                        write(client_fd, sptr, strlen(sptr));
                    }
                }
            }
            else if(strncmp(operation_arg, "pending", sizeof(operation_arg)) == 0)
            {
                if(g_queue_is_empty(q_pending)){
                    write(client_fd, "", 0);
                }
                else{
                    for(i = 0; sptr = g_queue_peek_nth(q_pending, i); i++){
                        write(client_fd, sptr, strlen(sptr));
                    }
                }
            }
            else
            {
                strncpy(buffer, "Wrong argument", sizeof(buffer));
                write(client_fd, buffer, strnlen(buffer, sizeof(buffer)));
            }
        }
        else if(strncmp(operation, "run", sizeof(operation)) == 0)
        {
            if(g_queue_is_empty(q_pending)){
                    write(client_fd, "", 0);
            }
            else{
                for(i = 0; sptr = g_queue_peek_nth(q_pending, i); i++){
                    write(client_fd, sptr, strlen(sptr));
                }
            }
        }
        else
        {
            strncpy(buffer, "Unspecified operation", sizeof(buffer));
            write(client_fd, buffer, strnlen(buffer, sizeof(buffer)));
        }

        close(client_fd);
    }

    close(server_fd);

    return 0;
}

void init(GTree* ruleTree)
{
    FILE* fptr;
    char program[20];
    char permission[20];
    char *tok;
    char temp[40];
    
    fptr = fopen("rule.table", "r");
    if(fptr == NULL)
        exit(EXIT_FAILURE);
    while(fgets(temp, sizeof(temp), fptr))
    {
        tok = strtok(temp, "\t");
        strncpy(program, tok, sizeof(program));
        tok = strtok(NULL, "\t");
        strncpy(permission, tok, sizeof(permission));
        g_tree_insert(ruleTree, program, permission);
    }
    fclose(fptr);
}