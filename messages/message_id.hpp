#pragma once

#include <span>
#include <vector>

enum class message_id : unsigned char {
    START_DEVICE = 0,
    START_HOST,
    LOG_INFO = 'I',
    RECEIVED_PACKET = 'R',
    PACKET_TO_SEND = 'S',
};

template <typename T>
struct io {
    static T deserialize(std::span<const unsigned char> buffer) {
        return io<T>{}.deserialize(buffer.subspan(1));
    }
};

