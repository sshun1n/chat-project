#include "protocol.h"
#include <string.h>
#include <arpa/inet.h>

void serialize_packet(const ChatPacket* packet, uint8_t* buffer) {
    size_t offset = 0;
    memcpy(buffer + offset, packet->username, MAX_USERNAME_LEN);
    offset += MAX_USERNAME_LEN;

    memcpy(buffer + offset, packet->color, 3);
    offset += 3;

    uint32_t msg_len = htonl(packet->msg_len);
    memcpy(buffer + offset, &msg_len, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, packet->iv, AES_BLOCK_SIZE);
    offset += AES_BLOCK_SIZE;

    memcpy(buffer + offset, packet->encrypted_msg, packet->msg_len);
}

void deserialize_packet(const uint8_t* buffer, ChatPacket* packet) {
    size_t offset = 0;
    memcpy(packet->username, buffer + offset, MAX_USERNAME_LEN);
    offset += MAX_USERNAME_LEN;

    memcpy(packet->color, buffer + offset, 3);
    offset += 3;

    uint32_t msg_len;
    memcpy(&msg_len, buffer + offset, sizeof(uint32_t));
    packet->msg_len = ntohl(msg_len);
    offset += sizeof(uint32_t);

    memcpy(packet->iv, buffer + offset, AES_BLOCK_SIZE);
    offset += AES_BLOCK_SIZE;

    memcpy(packet->encrypted_msg, buffer + offset, packet->msg_len);
}
