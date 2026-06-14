#pragma once

#include <array>
#include <asio.hpp>

template<size_t buffer_size>
struct packet_buffer {
    std::array<uint8_t, buffer_size> buffer;
    size_t size;

    asio::const_buffer as_buffer() const {
        return asio::const_buffer(buffer.data(), size);
    }
};