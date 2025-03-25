#pragma once
#include "protocol.h"
#include <openssl/evp.h>

void encrypt_message(
    const uint8_t* key,
    uint8_t* iv,
    const char* plaintext,
    uint8_t* ciphertext,
    uint32_t* len
);

void decrypt_message(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* ciphertext,
    uint32_t ciphertext_len,
    char* plaintext
);
