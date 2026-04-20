#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    int sock;
    int logged;
} Client;

Client clients[MAX_CLIENTS];

int check_login(char *user, char *pass) {
    FILE *f = fopen("database.txt", "r");
    if (!f) return 0;

    char u[50], p[50];

    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void send_file(int sock) {
    FILE *f = fopen("out.txt", "r");
    if (!f) {
        send(sock, "khong mo duoc file\n", 22, 0);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        send(sock, line, strlen(line), 0);
    }

    fclose(f);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t addrlen = sizeof(client_address);

    char buffer[BUFFER_SIZE];

    struct pollfd fds[MAX_CLIENTS + 1];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].logged = 0;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_socket, 5);

    printf("telnet server running on port 8080\n");

    while (1) {
        int nfds = 0;

        // server socket
        fds[nfds].fd = server_socket;
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

        // kết nối mới
        if (fds[index].revents & POLLIN) {
            new_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = new_socket;
                    clients[i].logged = 0;

                    char msg[] = "nhap: user pass\n";
                    send(new_socket, msg, strlen(msg), 0);
                    break;
                }
            }
        }

        index++;

        // xử lý client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock == 0) continue;

            if (fds[index].revents & POLLIN) {
                int sd = clients[i].sock;
                int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

                if (valread <= 0) {
                    close(sd);
                    clients[i].sock = 0;
                    clients[i].logged = 0;
                    index++;
                    continue;
                }

                buffer[valread] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;

                if (strlen(buffer) == 0) {
                    index++;
                    continue;
                }

                // login
                if (!clients[i].logged) {
                    char user[50], pass[50];

                    if (sscanf(buffer, "%s %s", user, pass) == 2) {
                        if (check_login(user, pass)) {
                            clients[i].logged = 1;
                            send(sd, "login ok\n", 9, 0);
                        } else {
                            send(sd, "login fail\n", 11, 0);
                        }
                    } else {
                        send(sd, "nhap dung: user pass\n", 24, 0);
                    }
                }
                // command
                else {
                    char cmd[BUFFER_SIZE];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt", buffer);

                    system(cmd); // thực thi lệnh
                    send_file(sd); // gửi kết quả
                }
            }
            index++;
        }
    }

    close(server_socket);
    return 0;
}