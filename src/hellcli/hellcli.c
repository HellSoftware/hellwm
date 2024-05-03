/* client side of communication tool with the HellWM Wayland Compositor */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../../include/hellcli/hellcli.h"

#define HELLWM_SOCK "/tmp/hellwm" 

void hellcli_print_usage()
{
	// TODO add usage options under -h
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        hellcli_print_usage();
        exit(1);
    }
    char *buffer = argv[1];

    hellcli_print_usage();

    struct sockaddr_un server_addr;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, HELLWM_SOCK);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1)
    {
        perror("Cannot connect to hellwm socket");
        return 1;
    }

    write(sockfd, buffer, sizeof(buffer));
    printf("Sent: %s", buffer);
    
    if (0)
    {
        listen(sockfd, 2);
        read(sockfd, buffer, sizeof(buffer));

        /* if server returns 0 on first byte, then print error,
         * any other data will be just printed out.
         * Error info is stored in other rest of bytes normally.
         */
        if (buffer[0]==1)
        {
            printf("[HellWM]: \n%s",buffer);
        }
        else
        {
            strcpy(buffer, buffer+1); // TODO (haven't tested yet)
            printf("[ERROR]: %s",buffer);
        }
    }

    close(sockfd);
    return 0;
}
