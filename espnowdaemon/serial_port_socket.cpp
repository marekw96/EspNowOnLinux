#include "serial_port_socket.hpp"
#include <iostream>

serial_port_socket::serial_port_socket(asio::io_context& io_context, std::string_view device_file)
    : serial_port_(io_context) {
    serial_port_.open(std::string(device_file));
}

void serial_port_socket::set_read_handler(read_packet_handler_t read_handler) {
    read_handler_ = read_handler;
}

void serial_port_socket::set_write_handler(write_packet_handler_t write_handler) {
    write_handler_ = write_handler;
}

void serial_port_socket::start_reading() {
    auto available = buffer_.get_writable();
    serial_port_.async_read_some(asio::buffer(available.data(), available.size()),
        std::bind(&serial_port_socket::read_handle, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void serial_port_socket::write(std::span<const uint8_t> data) {
    packet_buffer<512> packet;

    packet.size = data.size();
    memcpy(packet.buffer.data(), data.data(), data.size());

    write_queue_.push_back(packet);

    start_writing();
}

void serial_port_socket::start_writing() {
    if (write_queue_.empty()) return;

    serial_port_.async_write_some(write_queue_.front().as_buffer(),
        std::bind(&serial_port_socket::write_handle, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void serial_port_socket::read_handle(const asio::error_code& ec, size_t bytes_transferred) {
    if(ec && read_handler_) {
        read_handler_(ec, {});
        return;
    }

    buffer_.commit(bytes_transferred);
    auto data = buffer_.get_data();
    while(!data.empty()){
        auto processed = read_handler_(ec, data);
        if(processed == -1) {
            buffer_.move_data_to_front();
            break;
        }

        if(processed == 0){
            std::cerr << "Error handling serial packet" << std::endl;
            break;
        }

        buffer_.consume(processed);
        data = buffer_.get_data();
    }

    buffer_.move_data_to_front();
    start_reading();
}

void serial_port_socket::write_handle(const asio::error_code& ec, size_t bytes_transferred) {
    if(write_handler_) {
        write_handler_(ec, {});
    }

    if(!write_queue_.empty()) {
        write_queue_.pop_front();
        start_writing();
    }
}