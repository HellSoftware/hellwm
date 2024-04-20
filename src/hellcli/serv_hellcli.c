
/* Server side of communication tool with the HellWM Wayland Compositor */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define HELLWM_SOCK "/tmp/hellwm"

/*
 * for example: hellcli kill active 
 * kill - group_keyword of hellcli_cmd structure
 * active - pointer 'var' in hellcli_cmd_filler
 *
 * we will assign destroy_toplevel() to the assignedCommand
 * and put 'active' toplevel provided from server
 */

void test_func()
{
    printf("Testing\n");
}

typedef void(*assignedCommand)(void);

typedef struct {
    char *group_keyword;
    char *name;
    void *variable;
    assignedCommand cmd;
} hellcli_cmd;

// temp. main function for testing purpose
int main(int argc, char *argv[])
{
    hellcli_cmd cmd;
    cmd.cmd = test_func; 
    
    cmd.cmd();

    int buffer_size = 32;
    int c_sockfd;
    int s_sockfd;
    int s_size;

    char buffer[buffer_size];
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;
    
    s_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path,HELLWM_SOCK);
    s_size = sizeof(server_addr);
    
    if (bind(s_sockfd, (struct sockaddr *) &server_addr,s_size)==-1)
    {
        perror("Failed to bind socket");
        return 1;
    }

    listen(s_sockfd, 10);

    while (1)
    {
        int c_size = sizeof(client_addr);
        c_sockfd = accept(s_sockfd, (struct sockaddr *) &client_addr, &c_size); 

        read(c_sockfd, &buffer, buffer_size);
        printf("Server: %s\n", buffer);

        /* if errors, send info back to buffer */

        close(c_sockfd);
    }
        
    close(s_sockfd);
    return 0;
}
