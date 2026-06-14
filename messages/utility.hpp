#pragma once

#include <arpa/inet.h>
#include <span>

static inline char host_to_network(char value) {
    return value;
}

static inline char network_to_host(char value) {
    return value;
}

static inline uint8_t host_to_network(uint8_t value) {
    return value;
}

static inline uint8_t network_to_host(uint8_t value) {
    return value;
}

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

template <typename T>
struct basic_buffer_utils {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, T& value) {
        assert(buffer.size() >= sizeof(T));

        for(auto i = 0; i < sizeof(T); ++i) {
            value <<= 8;
            value |= buffer[i];
        }

        value = network_to_host(value);

        return buffer.subspan(sizeof(T));
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, T value) {
        assert(buffer.size() >= sizeof(T));

        value = host_to_network(value);

        for(auto i = 0; i < sizeof(T); ++i) {
            buffer[sizeof(T) - 1 - i] = value & 0xFF;
            value >>= 8;
        }

        return buffer.subspan(sizeof(T));
    }
};

template <typename T, size_t S>
struct basic_buffer_utils_array {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, T(&value)[S]) {
        assert(buffer.size() >= S * sizeof(T));

        for (size_t i = 0; i < S; ++i) {
            buffer = buffer_utils<T>::read(buffer, value[i]);
        }

        return buffer;
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, const T(&value)[S]) {
        assert(buffer.size() >= S * sizeof(T));

        for (size_t i = 0; i < S; ++i) {
            buffer = buffer_utils<T>::write(buffer, value[i]);
        }

        return buffer;
    }
};

template<>
struct buffer_utils<uint8_t> : basic_buffer_utils<uint8_t> {
};

template<>
struct buffer_utils<char> : basic_buffer_utils<char> {
};

template<size_t S>
struct buffer_utils<char[S]> : basic_buffer_utils_array<char, S> {
};

template <size_t S>
struct buffer_utils<uint8_t[S]> : basic_buffer_utils_array<uint8_t, S> {
};

template <>
struct buffer_utils<uint32_t> : basic_buffer_utils<uint32_t> {
};

template <>
struct buffer_utils<uint16_t> : basic_buffer_utils<uint16_t> {
};