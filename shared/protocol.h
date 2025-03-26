#pragma once
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <stdint.h>

#define MAX_USERNAME_LEN 32
#define MAX_MSG_LEN 256
#define AES_KEY_SIZE 256

typedef struct {
    char username[MAX_USERNAME_LEN];
    uint8_t color[3];
    uint8_t iv[EVP_MAX_IV_LENGTH];
    uint8_t encrypted_msg[MAX_MSG_LEN];
    uint32_t msg_len;
} ChatPacket;

void serialize_packet(const ChatPacket* packet, uint8_t* buffer);
void deserialize_packet(const uint8_t* buffer, ChatPacket* packet);
