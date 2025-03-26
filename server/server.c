#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include "../shared/protocol.h"
#include "../shared/crypto.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE sizeof(ChatPacket)

int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_packet(const ChatPacket* packet) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    FILE* log_file = fopen("server.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] Received encrypted packet from %s\n", 
                timestamp, packet->username);
        fclose(log_file);
    }
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    uint8_t buffer[BUFFER_SIZE];

    while (1) {
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) break;

        ChatPacket packet;
        deserialize_packet(buffer, &packet);
        
        // Логирование зашифрованного пакета
        log_packet(&packet);

        // Пересылка всем клиентам без изменений
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0 && client_sockets[i] != client_fd) {
                write(client_sockets[i], buffer, BUFFER_SIZE);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(client_fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_fd) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server started on port 8080\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_fd;
		printf("Client [%d] succesfully connected\n", i);
                pthread_t thread;
                pthread_create(&thread, NULL, handle_client, &client_fd);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return 0;
}
