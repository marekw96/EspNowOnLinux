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

template<typename T>
struct buffer_utils {
};

template<>
struct buffer_utils<uint8_t> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, uint8_t& value) {
        assert(buffer.size() >= 1);
        value = buffer[0];
        return buffer.subspan(1);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, uint8_t value) {
        assert(buffer.size() >= 1);
        buffer[0] = value;
        return buffer.subspan(1);
    }
};

template<>
struct buffer_utils<char> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, char& value) {
        assert(buffer.size() >= 1);
        value = buffer[0];
        return buffer.subspan(1);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, char value) {
        assert(buffer.size() >= 1);
        buffer[0] = value;
        return buffer.subspan(1);
    }
};

template<size_t S>
struct buffer_utils<char[S]> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, char(&value)[S]) {
        assert(buffer.size() >= S);

        for (size_t i = 0; i < S; ++i) {
            value[i] = buffer[i];
        }

        return buffer.subspan(S);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, const char(&value)[S]) {
        assert(buffer.size() >= S);

        for (size_t i = 0; i < S; ++i) {
            buffer[i] = value[i];
        }

        return buffer.subspan(S);
    }
};

template <size_t S>
struct buffer_utils<uint8_t[S]> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, uint8_t(&value)[S]) {
        assert(buffer.size() >= S);

        for (size_t i = 0; i < S; ++i) {
            value[i] = buffer[i];
        }

        return buffer.subspan(S);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, const uint8_t(&value)[S]) {
        assert(buffer.size() >= S);

        for (size_t i = 0; i < S; ++i) {
            buffer[i] = value[i];
        }

        return buffer.subspan(S);
    }
};

template <>
struct buffer_utils<uint32_t> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, uint32_t& value) {
        assert(buffer.size() >= 4);

        value = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        value = network_to_host(value);

        return buffer.subspan(4);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, uint32_t value) {
        assert(buffer.size() >= 4);

        uint32_t net_value = host_to_network(value);
        buffer[0] = (net_value >> 24) & 0xFF;
        buffer[1] = (net_value >> 16) & 0xFF;
        buffer[2] = (net_value >> 8) & 0xFF;
        buffer[3] = net_value & 0xFF;

        return buffer.subspan(4);
    }
};

template <>
struct buffer_utils<uint16_t> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, uint16_t& value) {
        assert(buffer.size() >= 2);

        value = (buffer[0] << 8) | buffer[1];
        value = network_to_host(value);

        return buffer.subspan(2);
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, uint16_t value) {
        assert(buffer.size() >= 2);

        uint16_t net_value = host_to_network(value);
        buffer[0] = (net_value >> 8) & 0xFF;
        buffer[1] = net_value & 0xFF;

        return buffer.subspan(2);
    }
};