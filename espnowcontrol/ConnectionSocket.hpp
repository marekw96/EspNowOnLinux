#pragma once

#include <asio.hpp>
#include <vector>
#include <cstdint>

class ConnectionSocket {
    public:
        ConnectionSocket(asio::io_context& io_context);
        void send_message(std::span<const uint8_t> message);
        std::vector<uint8_t> receive_message();

    private:
        asio::io_context& io_context_;
        asio::ip::tcp::socket socket_;
};