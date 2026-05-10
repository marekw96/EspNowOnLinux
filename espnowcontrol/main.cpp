#include <iostream>
#include <string>
#include <asio.hpp>

constexpr auto CONTROL_PORT = 19997;
constexpr auto CONTROL_IP = "localhost";

int main(int argc, char *argv[]) {
    try {
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(CONTROL_IP, std::to_string(CONTROL_PORT));

        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        std::cout << "Connected to " << CONTROL_IP << ":" << CONTROL_PORT << std::endl;

        std::array<char, 1024> data;
        std::error_code error;
        auto received_size = socket.read_some(asio::buffer(data), error);

        if (error == asio::error::eof) {
            std::cerr << "Connection closed by peer" << std::endl;
        } else if (error) {
            throw std::system_error(error);
        }

        std::cout << "Received " << received_size << " bytes" << std::endl;
        std::cout << "Data: " << std::string(data.data(), received_size) << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
