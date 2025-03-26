#pragma once
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <stdint.h>

#define MAX_USERNAME_LEN 32
#define MAX_MSG_LEN 256
#define AES_KEY_SIZE 256

// Коды цветов ncurses
typedef enum {
    NC_BLACK = 0,
    NC_RED,
    NC_GREEN,
    NC_YELLOW,
    NC_BLUE,
    NC_MAGENTA,
    NC_CYAN,
    NC_WHITE
} NcursesColor;

typedef struct {
    char username[MAX_USERNAME_LEN];
    uint8_t color; // Используем enum NcursesColor
    uint8_t iv[EVP_MAX_IV_LENGTH];
    uint8_t encrypted_msg[MAX_MSG_LEN];
    uint32_t msg_len;
} ChatPacket;

void serialize_packet(const ChatPacket* packet, uint8_t* buffer);
void deserialize_packet(const uint8_t* buffer, ChatPacket* packet);