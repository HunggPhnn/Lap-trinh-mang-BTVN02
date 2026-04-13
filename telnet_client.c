#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 2048

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("connect failed\n");
        return 1;
    }

    // nhan message tu server
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

    // nhan ket qua login
    len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[len] = '\0';
    printf("%s", buffer);

    if (strstr(buffer, "login ok") == NULL) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    // command loop 
    while (1) {
        printf("cmd> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0)
            break;

        send(sock, buffer, strlen(buffer), 0);

        // nhan ket qua (nhieu lan)
        while (1) {
            len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (len <= 0) break;

            buffer[len] = '\0';
            printf("%s", buffer);

            if (len < BUFFER_SIZE - 1) break; // het du lieu
        }

        printf("\n");
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}