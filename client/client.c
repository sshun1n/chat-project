#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <stdbool.h>
#include <openssl/rand.h>
#include "ui.h"
#include "../shared/protocol.h"
#include "../shared/crypto.h"
#include "../shared/config.h"

#define BUFFER_SIZE sizeof(ChatPacket)

int sock;
bool running = true;
wchar_t username[MAX_USERNAME_LEN];
uint8_t current_color = NC_WHITE;

// Функция для проверки корректности IP-адреса
bool is_valid_ip(const char* ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1;
}

// Функция для проверки корректности порта
bool is_valid_port(int port) {
    return port > 0 && port < 65536;
}

// Функция для получения IP-адреса и порта от пользователя
void get_connection_details(char* ip, int* port) {
    char input[IP_ADDR_LEN];
    char default_ip[] = "127.0.0.1";
    
    // Получаем IP-адрес
    printf("Enter server IP address [%s]: ", default_ip);
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Удаляем символ новой строки
        input[strcspn(input, "\n")] = 0;
        
        // Если строка пустая, используем значение по умолчанию
        if (strlen(input) == 0) {
            strcpy(ip, default_ip);
        } else {
            strncpy(ip, input, IP_ADDR_LEN - 1);
            ip[IP_ADDR_LEN - 1] = '\0';
        }
    } else {
        strcpy(ip, default_ip);
    }
    
    // Проверяем корректность IP-адреса
    while (!is_valid_ip(ip)) {
        printf("Invalid IP address. Please enter a valid IP (e.g., 127.0.0.1): ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0;
            strncpy(ip, input, IP_ADDR_LEN - 1);
            ip[IP_ADDR_LEN - 1] = '\0';
        }
    }
    
    // Получаем порт
    char port_str[IP_ADDR_LEN];
    printf("Enter server port [%d]: ", DEFAULT_PORT);
    fflush(stdout);
    
    if (fgets(port_str, sizeof(port_str), stdin) != NULL) {
        port_str[strcspn(port_str, "\n")] = 0;
        
        // Если строка пустая, используем порт по умолчанию
        if (strlen(port_str) == 0) {
            *port = DEFAULT_PORT;
        } else {
            *port = atoi(port_str);
        }
    } else {
        *port = DEFAULT_PORT;
    }
    
    // Проверяем корректность порта
    while (!is_valid_port(*port)) {
        printf("Invalid port number. Please enter a valid port (1-65535): ");
        fflush(stdout);
        if (fgets(port_str, sizeof(port_str), stdin) != NULL) {
            port_str[strcspn(port_str, "\n")] = 0;
            *port = atoi(port_str);
        }
    }
}


void send_message(int sock, const wchar_t* msg) {
    ChatPacket packet;
    wcstombs(packet.username, username, MAX_USERNAME_LEN);
    
    packet.color = current_color; 
    
    RAND_bytes(packet.iv, EVP_MAX_IV_LENGTH);
    char utf8_msg[MAX_MSG_LEN];
    wcstombs(utf8_msg, msg, MAX_MSG_LEN);
    encrypt_message(MY_AES_KEY, packet.iv, utf8_msg, 
                   packet.encrypted_msg, &packet.msg_len);

    uint8_t buffer[BUFFER_SIZE];
    serialize_packet(&packet, buffer);
    write(sock, buffer, BUFFER_SIZE);
}

void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    uint8_t buffer[BUFFER_SIZE];

    while (running) {
        int bytes_read = read(sock, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            running = false;
            break;
        }

        ChatPacket packet;
        deserialize_packet(buffer, &packet);

        wchar_t wusername[MAX_USERNAME_LEN];
        mbstowcs(wusername, packet.username, MAX_USERNAME_LEN);
        
        char plaintext[MAX_MSG_LEN];
        decrypt_message(MY_AES_KEY, packet.iv, 
                       packet.encrypted_msg, packet.msg_len, plaintext);
        
        wchar_t wplaintext[MAX_MSG_LEN];
        mbstowcs(wplaintext, plaintext, MAX_MSG_LEN);
        add_to_history(wusername, wplaintext, packet.color);
        display_history();
    }
    return NULL;
}

int main() {
    setlocale(LC_ALL, "");

    char server_ip[IP_ADDR_LEN];
    int server_port;
    get_connection_details(server_ip, &server_port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(server_port),
        .sin_addr.s_addr = inet_addr(server_ip)
    };

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    const char* env_user = getenv("USER");
    if (env_user && strlen(env_user)) {
        mbstowcs(username, env_user, MAX_USERNAME_LEN);
    } else {
        wcscpy(username, L"unknown");
    }
    username[MAX_USERNAME_LEN-1] = L'\0';

    init_ui();
    add_to_history(L"System", L"Connected to chat!", NC_GREEN);
    display_history();

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &sock);

    input_loop();

    endwin();
    close(sock);
    printf("Disconnected\n");
    return 0;
}