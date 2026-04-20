#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

typedef struct {
    int sock;
    char id[50];
    int named;
} Client;

Client clients[MAX_CLIENTS];

void broadcast(char *msg, int sender) {
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
    int master_sock, new_sock;
    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    char buffer[BUFFER_SIZE];

    struct pollfd fds[MAX_CLIENTS + 1];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
    }

    master_sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    bind(master_sock, (struct sockaddr*)&server, sizeof(server));
    listen(master_sock, 5);

    printf("Chat server running on port 8888...\n");

    while (1) {
        int nfds = 0;

        // master socket
        fds[nfds].fd = master_sock;
        fds[nfds].events = POLLIN;
        nfds++;

        // client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock != 0) {
                fds[nfds].fd = clients[i].sock;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int activity = poll(fds, nfds, -1);

        if (activity < 0) {
            perror("poll error");
            break;
        }

        int index = 0;

        // check master socket
        if (fds[index].revents & POLLIN) {
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

        index++;

        // check clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock == 0)
                continue;

            if (fds[index].revents & POLLIN) {
                int sd = clients[i].sock;
                int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

                if (valread <= 0) {
                    close(sd);
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
                    } else {
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
            index++;
        }
    }

    close(master_sock);
    return 0;
}