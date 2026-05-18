#pragma once

#include <span>
#include <cstring>

template <size_t capacity>
class receiving_buffer {
public:
    std::span<uint8_t> get_writable() {
        return std::span<uint8_t>(buffer + write_index, capacity - write_index);
    }

    std::span<uint8_t> get_data() {
        return std::span<uint8_t>(buffer + read_index, write_index - read_index);
    }

    void commit(size_t amount) {
        write_index += amount;
    }

    void consume(size_t amount) {
        read_index += amount;
    }

    void reset() {
        read_index = 0;
        write_index = 0;
    }

    void move_data_to_front() {
        if (read_index > 0) {
            std::memmove(buffer, buffer + read_index, write_index - read_index);
            write_index -= read_index;
            read_index = 0;
        }
    }

private:
    uint8_t buffer[capacity];
    size_t read_index = 0;
    size_t write_index = 0;
};