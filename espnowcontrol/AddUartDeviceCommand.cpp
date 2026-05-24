#include "AddUartDeviceCommand.hpp"
#include <iostream>
#include <string>

namespace{
    std::string make_add_uart_device_command(std::string_view device_file) {
        std::string command = "{\"action\" : \"add_uart_device\",\"device_file\" : \"" + std::string(device_file) + "\"}";
        return command;
    }
}

std::string_view AddUartDeviceCommand::get_name() const {
    return "add_uart_device";
}

std::string_view AddUartDeviceCommand::get_description() const {
    return "add_uart_device <device_file> - adds a new uart device";
}

bool AddUartDeviceCommand::handle(args_t args, asio::ip::tcp::socket& socket) {
    if (args.size() != 1) {
        std::cerr << "Usage: add_uart_device <device_file>" << std::endl;
        return false;
    }

    auto command = make_add_uart_device_command(args[0]);
    auto written_size = asio::write(socket, asio::buffer(command));
    std::error_code error;

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

    return true;
}