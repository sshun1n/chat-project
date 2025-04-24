#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "../shared/protocol.h"
#include "../shared/config.h"

#define BUFFER_SIZE sizeof(ChatPacket)

typedef struct {
    int socket;
    char ip[IP_ADDR_LEN];
    char username[32];
    int id;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int server_running = 1;  // Флаг для контроля работы сервера
int server_socket;  // Глобальный дескриптор серверного сокета

void log_message(const char* format, ...) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    printf("[%s]: %s\n", timestamp, message);
    
    FILE* log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "[%s]: %s\n", timestamp, message);
        fclose(log_file);
    }
}

void print_clients_table() {
    printf("\nConnected Clients:\n");
    printf("ID | IP Address\n");
    printf("-------------------\n");
    
    pthread_mutex_lock(&clients_mutex);
    int active_clients = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) {
            printf("%2d | %s\n", i, clients[i].ip);
            active_clients++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    if (active_clients == 0) {
        printf("No clients connected\n");
    }
    printf("-------------------\n");
}

void print_help() {
    printf("\nAvailable commands:\n");
    printf("list   - Show connected clients\n");
    printf("help   - Show this help message\n");
    printf("exit   - Safely shutdown the server\n");
    printf("-------------------\n");
}

// Функция очистки при выходе
void cleanup_server() {
    server_running = 0;
    
    // Закрываем все клиентские соединения
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) {
            close(clients[i].socket);
            clients[i].socket = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Закрываем серверный сокет
    if (server_socket > 0) {
        close(server_socket);
    }
    
    log_message("Server shutdown completed");
}

void* handle_client(void* arg) {
    ClientInfo* client = (ClientInfo*)arg;
    int client_fd = client->socket;
    uint8_t buffer[BUFFER_SIZE];

    while (server_running) {
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            log_message("Client disconnected [%s]", client->ip);
            break;
        }

        ChatPacket packet;
        deserialize_packet(buffer, &packet);
        
        log_message("Received encrypted packet from [%s][%s]", 
                   client->ip, packet.username);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0 && clients[i].socket != client_fd) {
                if (write(clients[i].socket, buffer, BUFFER_SIZE) < 0) {
                    log_message("Failed to send message to client %s", clients[i].ip);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(client_fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_fd) {
            clients[i].socket = 0;
            memset(clients[i].ip, 0, IP_ADDR_LEN);
            memset(clients[i].username, 0, 32);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

void* handle_commands(void* arg) {
    char command[32];
    print_help();
    
    while (server_running) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            if (feof(stdin)) {
                break;
            }
            continue;
        }
        
        // Удаляем символ новой строки
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "list") == 0) {
            print_clients_table();
        }
        else if (strcmp(command, "help") == 0) {
            print_help();
        }
        else if (strcmp(command, "exit") == 0) {
            log_message("Server shutdown initiated");
            cleanup_server();
            exit(0);
        }
        else if (strlen(command) > 0) {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
    return NULL;
}

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n");
        log_message("Received shutdown signal");
        cleanup_server();
        _exit(0);
    }
}

int main() {
    // Установка обработчиков сигналов
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Инициализация массива клиентов
    memset(clients, 0, sizeof(clients));

    // Создаем сокет
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // Устанавливаем опцию переиспользования адреса
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DEFAULT_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    // Привязываем сокет
    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind");
        return 1;
    }

    // Начинаем прослушивание
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen");
        return 1;
    }

    log_message("Server started on port %d", DEFAULT_PORT);

    // Создаем поток для обработки команд
    pthread_t command_thread;
    if (pthread_create(&command_thread, NULL, handle_commands, NULL) != 0) {
        log_message("Failed to create command thread");
        return 1;
    }

    // Отсоединяем поток команд, чтобы он работал независимо
    pthread_detach(command_thread);

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);

        if (!server_running) break;

        if (client_fd < 0) {
            perror("Failed to accept connection");
            continue;
        }

        // Получаем IP клиента
        char client_ip[IP_ADDR_LEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, IP_ADDR_LEN);

        pthread_mutex_lock(&clients_mutex);
        
        // Проверяем, есть ли свободные слоты
        int slot_found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = client_fd;
                clients[i].id = i;
                strncpy(clients[i].ip, client_ip, IP_ADDR_LEN - 1);
                clients[i].ip[IP_ADDR_LEN - 1] = '\0';
                
                log_message("Client connected [%s]", clients[i].ip);
                
                pthread_t thread;
                ClientInfo* client_info = &clients[i];
                if (pthread_create(&thread, NULL, handle_client, client_info) != 0) {
                    perror("Failed to create thread for client");
                    close(client_fd);
                    clients[i].socket = 0;
                } else {
                    pthread_detach(thread);
                }
                slot_found = 1;
                break;
            }
        }
        
        if (!slot_found) {
            log_message("Server is full, connection from %s rejected", client_ip);
            close(client_fd);
        }
        
        pthread_mutex_unlock(&clients_mutex);
    }

    cleanup_server();
    return 0;
}
