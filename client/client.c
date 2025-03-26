#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <openssl/rand.h>

#include "../shared/config.h"
#include "../shared/protocol.h"
#include "../shared/crypto.h"

#define BUFFER_SIZE sizeof(ChatPacket)
#define CLEAR_LINE "\033[2K\r"

static const uint8_t* aes_key = MY_AES_KEY;
char username[MAX_USERNAME_LEN] = "User";
uint8_t color[3] = {255, 255, 255}; // Белый по умолчанию
bool running = true;

void print_help() {
    printf("\nAvailable commands:\n");
    printf("/color <R> <G> <B>  - Change text color (0-255 values)\n");
    printf("/name <new_name>    - Change your username\n");
    printf("/help               - Show this help message\n");
    printf("/exit               - Exit the chat\n\n");
}

void display_message(const ChatPacket* packet, const char* plaintext) {
    printf(CLEAR_LINE);
    printf("\033[38;2;%d;%d;%dm%s:\033[0m %s\n", 
           packet->color[0], packet->color[1], packet->color[2],
           packet->username, plaintext);
    printf("> ");
    fflush(stdout);
}

void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    uint8_t buffer[BUFFER_SIZE];

    while (running) {
        int bytes_read = read(sock, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            printf("\nConnection lost\n");
            exit(EXIT_FAILURE);
        }

        ChatPacket packet;
        deserialize_packet(buffer, &packet);
        
        // Дешифруем сообщение
        char plaintext[MAX_MSG_LEN];
        decrypt_message(aes_key, packet.iv, 
                       packet.encrypted_msg, packet.msg_len, plaintext);
        display_message(&packet, plaintext); // Передаём расшифрованный текст
    }
    return NULL;
}

void send_message(int sock, const char* msg) {
    ChatPacket packet;
    strncpy(packet.username, username, MAX_USERNAME_LEN);
    memcpy(packet.color, color, 3);
    
    //RAND_bytes(packet.iv, EVP_MAX_IV_LENGTH);
    encrypt_message(aes_key, packet.iv, msg, 
                   packet.encrypted_msg, &packet.msg_len);

    uint8_t buffer[BUFFER_SIZE];
    serialize_packet(&packet, buffer);
    write(sock, buffer, BUFFER_SIZE);
}

int main() {
    // Настройка сокета
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    // Подключение к серверу
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Автоматическое получение имени
    const char* env_user = getenv("USER");
    if (env_user && strlen(env_user) > 0) {
        size_t user_len = strlen(env_user);
        if (user_len >= MAX_USERNAME_LEN) printf("Username truncated to %d characters\n", MAX_USERNAME_LEN-1);
        strncpy(username, env_user, MAX_USERNAME_LEN);
    } 
    else strncpy(username, "unknown", MAX_USERNAME_LEN);

    username[MAX_USERNAME_LEN-1] = '\0';
    
    printf("Connected to server as \033[1;34m%s\033[0m\n", username);

    print_help();

    // Запуск потока для приема сообщений
    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &sock);

    // Главный цикл ввода
    char msg[MAX_MSG_LEN];
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(msg, MAX_MSG_LEN, stdin)) break;
        msg[strcspn(msg, "\n")] = 0;

        // Обработка команд
        if (msg[0] == '/') {
            if (strncmp(msg, "/color", 6) == 0) {
                uint8_t r, g, b;
                if (sscanf(msg + 7, "%hhu %hhu %hhu", &r, &g, &b) == 3) {
                    color[0] = r;
                    color[1] = g;
                    color[2] = b;
                    printf(CLEAR_LINE "Color changed!\n");
                }
            } 
            else if (strncmp(msg, "/name", 5) == 0) {
                strncpy(username, msg + 6, MAX_USERNAME_LEN);
                printf(CLEAR_LINE "Username changed to: %s\n", username);
            }
            else if (strncmp(msg, "/help", 5) == 0) print_help();
            else if (strncmp(msg, "/exit", 5) == 0) { running = false; break; }
            continue;
        }

        // Отправка обычного сообщения
        send_message(sock, msg);
        printf(CLEAR_LINE); // Очищаем строку с вводом
    }

    close(sock);
    printf("\nDisconnected\n");
    return 0;
}