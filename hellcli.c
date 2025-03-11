#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/hellwm.sock"
#define IPC_BUFFER_SIZE 256

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <cmd>\n", argv[0]);
        return 1;
    }
    int client_socket;
    struct sockaddr_un server_addr;
    char buffer[IPC_BUFFER_SIZE];

    client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    strcpy(buffer, argv[1]);
    write(client_socket, buffer, strlen(buffer));

    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0)
    {
        perror("Read error");
    }
    else
    {
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }

    close(client_socket);
    return 0;
}


