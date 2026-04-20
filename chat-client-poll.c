#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    struct pollfd fds[2];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    fds[0].fd = sock;     
    fds[0].events = POLLIN;

    fds[1].fd = STDIN_FILENO; 
    fds[1].events = POLLIN;

    printf("Connected to server...\n");

    while (1) {
        poll(fds, 2, -1);

        if (fds[0].revents & POLLIN) {
            int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (len <= 0) {
                printf("Server disconnected\n");
                break;
            }
            buffer[len] = '\0';
            printf("%s", buffer);
        }

        if (fds[1].revents & POLLIN) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}