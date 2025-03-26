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
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int outlen, tmplen;
    
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &outlen, 
                     (const uint8_t*)plaintext, strlen(plaintext)+1);
    EVP_EncryptFinal_ex(ctx, ciphertext + outlen, &tmplen);
    
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
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int outlen, tmplen;
    
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, (uint8_t*)plaintext, &outlen, 
                     ciphertext, ciphertext_len);
    EVP_DecryptFinal_ex(ctx, (uint8_t*)plaintext + outlen, &tmplen);
    
    plaintext[outlen + tmplen] = '\0';
    EVP_CIPHER_CTX_free(ctx);
}
