#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <openssl/rand.h>
#include "../shared/protocol.h"
#include "../shared/crypto.h"

#define BUFFER_SIZE sizeof(ChatPacket)

uint8_t aes_key[AES_KEY_SIZE/8];

void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    uint8_t buffer[BUFFER_SIZE];

    while (1) {
        int bytes_read = read(sock, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            printf("\nСоединение разорвано\n");
            exit(EXIT_FAILURE);
        }

        ChatPacket packet;
        deserialize_packet(buffer, &packet);

        char plaintext[MAX_MSG_LEN];
        decrypt_message(aes_key, packet.iv, packet.encrypted_msg, packet.msg_len, plaintext);

        // Перемещаем курсор на новую строку перед выводом
        printf("\x1b[38;2;%d;%d;%dm%s: %s\x1b[0m\n", 
               packet.color[0], packet.color[1], packet.color[2],
               packet.username, plaintext);

        fflush(stdout);
    }
    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
	
    memcpy(aes_key, "32-CHARACTER-KEY-1234567890-ABC", sizeof(aes_key));

    char username[MAX_USERNAME_LEN];
    printf("Enter your username: ");
    fgets(username, MAX_USERNAME_LEN, stdin);
    username[strcspn(username, "\n")] = 0;

    uint8_t color[3];
    printf("Enter RGB color (e.g., 255 0 0): ");
    scanf("%hhu %hhu %hhu", &color[0], &color[1], &color[2]);
    getchar();

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &sock);

    while (1) {
	fflush(stdout);
        char msg[MAX_MSG_LEN];
        fgets(msg, MAX_MSG_LEN, stdin);
        msg[strcspn(msg, "\n")] = 0;

        ChatPacket packet;
        strncpy(packet.username, username, MAX_USERNAME_LEN);
        memcpy(packet.color, color, 3);

        uint8_t iv[AES_BLOCK_SIZE];
        RAND_bytes(iv, AES_BLOCK_SIZE);
        encrypt_message(aes_key, iv, msg, packet.encrypted_msg, &packet.msg_len);
        memcpy(packet.iv, iv, AES_BLOCK_SIZE);

        uint8_t buffer[BUFFER_SIZE];
        serialize_packet(&packet, buffer);
        write(sock, buffer, BUFFER_SIZE);
    }

    close(sock);
    return 0;
}
