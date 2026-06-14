#include "GetDevicesListCommand.hpp"

#include <print>
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
        std::println("{}:", device.device_id());
        std::println("\tdevice name: {}", device.device_name());
        std::println("\tespnow version: {}", static_cast<int>(device.version()));
        std::print("\tmac address: ");
        for(const auto& byte : device.mac_address()) {
            std::print("{:02x}", static_cast<uint8_t>(byte));
            if(&byte != &device.mac_address().back()) {
                std::print(":");
            }
        }
        std::println();
        std::println("\tfw version: {}.{}.{}", device.fw_version().major(), device.fw_version().minor(), device.fw_version().patch());
        if(device.has_uart_info()) {
            std::println("\tconnection type: UART");
            std::println("\tdevice file: {}", device.uart_info().device_file());
        }
    }

    return true;
}