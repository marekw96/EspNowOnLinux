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

bool AddUartDeviceCommand::handle(ConnectionSocket& socket, args_t args) {
    if (args.size() != 1) {
        std::cerr << "Usage: add_uart_device <device_file>" << std::endl;
        return false;
    }

    auto command = make_add_uart_device_command(args[0]);
    socket.send_message(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(command.data()), command.size()));

    auto buffer = socket.receive_message();
    std::cout << "Received " << buffer.size() << " bytes" << std::endl;
    std::cout << "Data: " << std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size()) << std::endl;

    return true;
}