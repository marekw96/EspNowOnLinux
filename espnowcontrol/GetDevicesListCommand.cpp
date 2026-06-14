#include "GetDevicesListCommand.hpp"

#include <iostream>
#include <string>
#include "control_messages.pb.h"

std::string_view GetDevicesListCommand::get_name() const {
    return "devices";
}

std::string_view GetDevicesListCommand::get_description() const {
    return "devices - get list of devices";
}

bool GetDevicesListCommand::handle(ConnectionSocket& socket, args_t) {
    control_messages::control_envelope request_envelope;
    request_envelope.set_sequence_number(1);
    request_envelope.mutable_get_list_of_devices_request();

    std::string serialized_request;
    request_envelope.SerializeToString(&serialized_request);

    socket.send_message(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(serialized_request.c_str()), serialized_request.size()));

    auto message = socket.receive_message();

    control_messages::control_envelope response_envelope;
    response_envelope.ParseFromArray(message.data(), message.size());

    const auto& response = response_envelope.get_list_of_devices_response();

    for(const auto& device : response.devices()) {
        std::cout << device.device_id() << ":" << std::endl;
        std::cout << "\tdevice name: " << device.device_name() << std::endl;
        std::cout << "\tespnow version: " << device.version() << std::endl;
        std::cout << "\tmac address: ";
        for(const auto& byte : device.mac_address()) {
            std::cout << std::hex << static_cast<int>(static_cast<uint8_t>(byte)) << std::dec;
            if(&byte != &device.mac_address().back()) {
                std::cout << ":";
            }
        }
        std::cout << std::endl;
        std::cout << "\tfw version: " << device.fw_version().major() << "." << device.fw_version().minor() << "." << device.fw_version().patch() << std::endl;
        if(device.has_uart_info()) {
            std::cout << "\tconnection type: UART" << std::endl;
            std::cout << "\tdevice file: " << device.uart_info().device_file() << std::endl;
        }
    }

    return true;
}