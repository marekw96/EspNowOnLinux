#include "ConnectionSocket.hpp"

constexpr auto CONTROL_PORT = 19997;
constexpr auto CONTROL_IP = "localhost";

ConnectionSocket::ConnectionSocket(asio::io_context& io_context) : io_context_(io_context), socket_(io_context) {
    auto endpoints = asio::ip::tcp::resolver(io_context_).resolve(CONTROL_IP, std::to_string(CONTROL_PORT));
    asio::connect(socket_, endpoints);
}

void ConnectionSocket::send_message(std::span<const uint8_t> message) {
    asio::error_code error;
    asio::write(socket_, asio::buffer(message.data(), message.size()), error);

    if(error){
        throw std::runtime_error(error.message());
    }
}

std::vector<uint8_t> ConnectionSocket::receive_message() {
    std::vector<uint8_t> buffer(1024);
    asio::error_code error;

    socket_.read_some(asio::buffer(buffer), error);

    if(error){
        throw std::runtime_error(error.message());
    }

    return buffer;
}