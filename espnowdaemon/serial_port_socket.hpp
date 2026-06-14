#pragma once

#include <asio.hpp>
#include <string_view>
#include "utility/receiving_buffer.hpp"
#include "packet_buffer.hpp"
#include <functional>
#include <deque>
#include <utility>

using write_packet_handler_t = std::function<void(const asio::error_code& ec,size_t)>;
using read_packet_handler_t = std::function<int(const asio::error_code& ec,std::span<uint8_t>)>;

class serial_port_socket {
public:
    serial_port_socket(asio::io_context& io_context, std::string_view device_file);

    void set_read_handler(read_packet_handler_t read_handler);
    void set_write_handler(write_packet_handler_t write_handler);

    void write(std::span<const uint8_t> data);
    void start_reading();

private:
    asio::serial_port serial_port_;
    receiving_buffer<1024 * 4> buffer_;
    std::deque<packet_buffer<512>> write_queue_;

    read_packet_handler_t read_handler_;
    write_packet_handler_t write_handler_;

    void start_writing();

    void read_handle(const asio::error_code& ec, size_t bytes_transferred);
    void write_handle(const asio::error_code& ec, size_t bytes_transferred);
};