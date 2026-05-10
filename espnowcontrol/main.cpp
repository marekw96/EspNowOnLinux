#include <iostream>
#include <string>
#include <string_view>
#include <asio.hpp>

constexpr auto CONTROL_PORT = 19997;
constexpr auto CONTROL_IP = "localhost";

constexpr std::string_view json_add_uart_device = "{\"action\" : \"add_uart_device\",\"device_file\" : \"/dev/ttyACM0\"}";

int main(int argc, char *argv[]) {
    try {
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(CONTROL_IP, std::to_string(CONTROL_PORT));

        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        std::cout << "Connected to " << CONTROL_IP << ":" << CONTROL_PORT << std::endl;

        std::error_code error;
        auto written_size = asio::write(socket, asio::buffer(json_add_uart_device));

        if (error == asio::error::eof) {
            std::cerr << "Connection closed by peer" << std::endl;
            return 1;
        } else if (error) {
            throw std::system_error(error);
        }

        std::cout << "Written " << written_size << " bytes" << std::endl;

        std::array<char, 1024> data;
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
