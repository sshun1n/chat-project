#include "crypto.h"
#include <openssl/rand.h>
#include <openssl/err.h>
#include <string.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

void encrypt_message(
    const uint8_t* key,
    uint8_t* iv,
    const char* plaintext,
    uint8_t* ciphertext,
    uint32_t* len
) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int outlen, tmplen;

    if (!ctx) handle_openssl_error();

    if (RAND_bytes(iv, AES_BLOCK_SIZE) != 1)
        handle_openssl_error();

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
        handle_openssl_error();

    size_t plaintext_len = strlen(plaintext) + 1;

    if (EVP_EncryptUpdate(ctx, ciphertext, &outlen, 
                         (const uint8_t*)plaintext, plaintext_len) != 1)
        handle_openssl_error();

    if (EVP_EncryptFinal_ex(ctx, ciphertext + outlen, &tmplen) != 1)
        handle_openssl_error();

    *len = outlen + tmplen;
    EVP_CIPHER_CTX_free(ctx);
}

void decrypt_message(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* ciphertext,
    uint32_t ciphertext_len,
    char* plaintext
) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int outlen, tmplen;

    if (!ctx) handle_openssl_error();

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
        handle_openssl_error();

    if (EVP_DecryptUpdate(ctx, (uint8_t*)plaintext, &outlen, 
                         ciphertext, ciphertext_len) != 1)
        handle_openssl_error();

    if (EVP_DecryptFinal_ex(ctx, (uint8_t*)plaintext + outlen, &tmplen) != 1)
        handle_openssl_error();

    plaintext[outlen + tmplen] = '\0';
    EVP_CIPHER_CTX_free(ctx);
}
