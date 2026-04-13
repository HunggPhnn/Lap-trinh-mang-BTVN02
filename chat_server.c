#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

typedef struct {
    SOCKET sock;
    char id[50];
    int named;
} Client;

Client clients[MAX_CLIENTS];

void broadcast(char *msg, SOCKET sender) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock != 0 && clients[i].sock != sender) {
            send(clients[i].sock, msg, strlen(msg), 0);
        }
    }
}

void get_time_str(char *buf) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, 64, "%Y/%m/%d %I:%M:%S%p", t);
}

int main() {
    WSADATA wsa;
    SOCKET master_sock, new_sock, sd;
    struct sockaddr_in server, client;
    int addrlen = sizeof(client);

    fd_set readfds;
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
    }

    WSAStartup(MAKEWORD(2,2), &wsa);

    master_sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    bind(master_sock, (struct sockaddr*)&server, sizeof(server));
    listen(master_sock, 5);

    printf("Chat server running on port 8888...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_sock, &readfds);
        SOCKET max_sd = master_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].sock;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        select(0, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock, &readfds)) {
            new_sock = accept(master_sock, (struct sockaddr*)&client, &addrlen);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = new_sock;
                    clients[i].named = 0;

                    char msg[] = "Nhap theo format: client_id: client_name\n";
                    send(new_sock, msg, strlen(msg), 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].sock;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

                if (valread <= 0) {
                    closesocket(sd);
                    clients[i].sock = 0;
                    clients[i].named = 0;
                } else {
                    buffer[valread] = '\0';

                    buffer[strcspn(buffer, "\r\n")] = 0;

                    if (strlen(buffer) == 0)
                        continue;

                    if (!clients[i].named) {
                        char *p = strchr(buffer, ':');

                        if (p != NULL) {
                            *p = '\0';
                            strcpy(clients[i].id, buffer);
                            clients[i].named = 1;

                            send(sd, "OK\n", 3, 0);
                        } else {
                            char msg[] = "Sai cu phap. Nhap lai: client_id: client_name\n";
                            send(sd, msg, strlen(msg), 0);
                        }
                    }

                    else {
                        char msg[BUFFER_SIZE + 100];
                        char timebuf[64];

                        get_time_str(timebuf);

                        snprintf(msg, sizeof(msg),
                                 "%s %s: %s\n",
                                 timebuf,
                                 clients[i].id,
                                 buffer);

                        broadcast(msg, sd);
                    }
                }
            }
        }
    }

    closesocket(master_sock);
    WSACleanup();
    return 0;
}