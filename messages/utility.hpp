#pragma once

#include <arpa/inet.h>

static inline uint32_t host_to_network(uint32_t value) {
    return htonl(value);
}

static inline uint32_t network_to_host(uint32_t value) {
    return ntohl(value);
}

static inline uint16_t host_to_network(uint16_t value) {
    return htons(value);
}

static inline uint16_t network_to_host(uint16_t value) {
    return ntohs(value);
}