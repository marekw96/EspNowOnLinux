#include "AddUartDeviceCommand.hpp"
#include <iostream>
#include <print>
#include <string>
#include "control_messages.pb.h"

std::string_view AddUartDeviceCommand::get_name() const {
    return "add_uart_device";
}

std::string_view AddUartDeviceCommand::get_description() const {
    return "add_uart_device <device_file> - adds a new uart device";
}

bool AddUartDeviceCommand::handle(ConnectionSocket& socket, args_t args) {
    if (args.size() != 1) {
        std::println(std::cerr, "Usage: add_uart_device <device_file>");
        return false;
    }

    control_messages::control_envelope envelope;
    envelope.set_sequence_number(0);
    envelope.mutable_add_uart_device_request()->set_device_file(std::string(args[0]));

    std::string data;
    envelope.SerializeToString(&data);

    socket.send_message(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()));

    auto buffer = socket.receive_message();
    control_messages::control_envelope response_envelope;
    response_envelope.ParseFromArray(buffer.data(), buffer.size());

    if (response_envelope.has_add_uart_device_response()) {
        const auto& response = response_envelope.add_uart_device_response();
        if(response.status() == control_messages::add_uart_device_response::SUCCESS) {
            std::println("Added device with ID: {}", response.device_id());
        } else {
            switch(response.status()) {
                case control_messages::add_uart_device_response::DEVICE_FILE_DOES_NOT_EXIST:
                    std::println(std::cerr, "Device file does not exist");
                    break;
                case control_messages::add_uart_device_response::DEVICE_ALREADY_EXISTS:
                    std::println(std::cerr, "Device already exists");
                    break;
                case control_messages::add_uart_device_response::UNKNOWN_ERROR:
                    std::println(std::cerr, "Unknown error");
                    break;
            }
        }
    }

    return true;
}