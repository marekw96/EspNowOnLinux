#include "GetStatisticsCommand.hpp"
#include <iostream>
#include <string>
#include "control_messages.pb.h"

std::string_view GetStatisticsCommand::get_name() const {
    return "get_statistics";
}

std::string_view GetStatisticsCommand::get_description() const {
    return "get_statistics <device_name> - gets statistics for a device";
}

bool GetStatisticsCommand::handle(ConnectionSocket& socket, args_t args) {
    if (args.size() != 1) {
        std::cerr << "Usage: get_statistics <device_name>" << std::endl;
        return false;
    }

    control_messages::control_envelope envelope;
    envelope.set_sequence_number(0);
    envelope.mutable_get_statistics_request()->set_device_id(std::string(args[0]));

    std::string data;
    envelope.SerializeToString(&data);

    socket.send_message(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()));

    auto buffer = socket.receive_message();
    control_messages::control_envelope response_envelope;
    response_envelope.ParseFromArray(buffer.data(), buffer.size());

    if (response_envelope.has_get_statistics_response()) {
        const auto& response = response_envelope.get_statistics_response();
        if(response.status() == control_messages::get_statistics_response::SUCCESS) {
            std::cout << "Device statistics:" << std::endl;
            std::cout << "  Broadcast sent: " << response.broadcast_sent() << std::endl;
            std::cout << "  Broadcast received: " << response.broadcast_received() << std::endl;
        } else {
            switch(response.status()) {
                case control_messages::get_statistics_response::DEVICE_NOT_FOUND:
                    std::cerr << "Device not found" << std::endl;
                    break;
                case control_messages::get_statistics_response::UNKNOWN_ERROR:
                    std::cerr << "Unknown error" << std::endl;
                    break;
            }
        }
    }

    return true;
}