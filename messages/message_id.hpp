#pragma once

#include <span>
#include "utility.hpp"

enum class message_id : unsigned char {
    START_DEVICE = 0,
    START_HOST,
    LOG_INFO = 'I',
    LOG_INFO_UART = 'i',
    LOG_ERROR = 'R',
    RECEIVED_PACKET = 'R',
    PACKET_TO_SEND = 'S',
    PING = 'P'
};

template <typename T>
struct io {
    static T deserialize(std::span<const unsigned char> buffer) {
        return io<T>{}.deserialize(buffer.subspan(1));
    }
};

template<>
struct buffer_utils<message_id> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, message_id& value) {
        return buffer_utils<uint8_t>::read(buffer, reinterpret_cast<uint8_t&>(value));
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, message_id value) {
        return buffer_utils<uint8_t>::write(buffer, static_cast<uint8_t>(value));
    }
};

