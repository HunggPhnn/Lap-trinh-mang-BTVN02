#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 2048

int main() {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    struct pollfd fds[2];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return 1;
    }

    // nhận message đầu
    int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[len] = '\0';
    printf("%s", buffer);

    // login
    char user[50], pass[50];

    printf("user: ");
    fgets(user, sizeof(user), stdin);
    user[strcspn(user, "\n")] = 0;

    printf("pass: ");
    fgets(pass, sizeof(pass), stdin);
    pass[strcspn(pass, "\n")] = 0;

    snprintf(buffer, sizeof(buffer), "%s %s", user, pass);
    send(sock, buffer, strlen(buffer), 0);

    // nhận kết quả login
    len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[len] = '\0';
    printf("%s", buffer);

    if (strstr(buffer, "login ok") == NULL) {
        close(sock);
        return 0;
    }

    printf("Login thanh cong. Nhap lenh:\n");

    // setup poll
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (1) {
        poll(fds, 2, -1);

        // nhận từ server
        if (fds[0].revents & POLLIN) {
            len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (len <= 0) {
                printf("Server disconnected\n");
                break;
            }
            buffer[len] = '\0';
            printf("%s", buffer);
        }

        // nhập từ bàn phím
        if (fds[1].revents & POLLIN) {
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, "exit") == 0)
                break;

            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}