#include "control_session.hpp"
#include <iostream>
#include <functional>
#include "control_messages.pb.h"
#include "events/new_device_event.hpp"

control_session::pointer control_session::create(asio::io_context &io_context, device_manager& device_manager, event_dispatcher& dispatcher) {
    return std::make_shared<control_session>(io_context, device_manager, dispatcher);
}

control_session::control_session(asio::io_context &io_context, device_manager& device_manager, event_dispatcher& dispatcher)
    : socket_(io_context), device_manager_(device_manager), dispatcher_(dispatcher) {
    new_uart_device_subscriber_guard_ = dispatcher_.subscribe<new_device_event>(
        std::bind(&control_session::on_new_uart_device, this, std::placeholders::_1));
 }

asio::ip::tcp::socket &control_session::get_socket() { return socket_; }

void control_session::start() {
    std::cout << "New control session started" << std::endl;
    trigger_reading();
}

void control_session::trigger_reading(){
    socket_.async_read_some(asio::buffer(data_),
        std::bind(&control_session::handle_read, shared_from_this(),
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void control_session::handle_write(const std::error_code& ec, size_t bytes_transferred)
{}

void control_session::handle_read(const std::error_code& ec, size_t bytes_transferred)
{
    if (ec == asio::error::eof) {
        std::cout << "Client disconnected" << std::endl;
    } else if (ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
    } else {
        control_messages::control_envelope envelope;
        envelope.ParseFromArray(data_.data(), bytes_transferred);

        if(envelope.has_add_uart_device_request()){
            const auto& request = envelope.add_uart_device_request();
            std::cout << "Adding uart device: " << request.device_file() << std::endl;
            auto device = device_manager_.add_uart_device(request.device_file());

            control_messages::control_envelope response;
            response.set_sequence_number(envelope.sequence_number());
            auto* response_add = response.mutable_add_uart_device_response();

            if (!device) {
                std::cerr << "Error adding uart device: " << static_cast<int>(device.error()) << std::endl;
                if (device.error() == adding_device_error_code::device_file_does_not_exist) {
                    response_add->set_status(control_messages::add_uart_device_response::DEVICE_FILE_DOES_NOT_EXIST);
                } else {
                    response_add->set_status(control_messages::add_uart_device_response::UNKNOWN_ERROR);
                }
                response.SerializeToString(&response_);

                asio::async_write(socket_, asio::buffer(response_),
                    std::bind(&control_session::handle_write, shared_from_this(),
                        asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
        }
        else if(envelope.has_get_statistics_request()){
            control_messages::control_envelope response;
            response.set_sequence_number(envelope.sequence_number());
            auto* response_get = response.mutable_get_statistics_response();

            auto device_ref = device_manager_.get_device_by_id(envelope.get_statistics_request().device_id());

            if(device_ref.has_value()) {
                const auto& device = device_ref.value().get();
                auto stats = device.get_statistics();

                response_get->set_status(control_messages::get_statistics_response::SUCCESS);
                response_get->set_uptime_seconds(device.get_uptime().count());
                response_get->set_broadcast_sent(stats.broadcast_sent);
                response_get->set_broadcast_received(stats.broadcast_received);
            } else {
                response_get->set_status(control_messages::get_statistics_response::DEVICE_NOT_FOUND);
            }
            response.SerializeToString(&response_);

            asio::async_write(socket_, asio::buffer(response_),
                std::bind(&control_session::handle_write, shared_from_this(),
                    asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        else if(envelope.has_get_list_of_devices_request()){
            control_messages::control_envelope response;
            response.set_sequence_number(envelope.sequence_number());
            auto* response_get = response.mutable_get_list_of_devices_response();

            for(const auto& device : device_manager_.get_devices()) {
                auto* device_info = response_get->add_devices();
                device_info->set_device_id(std::string(device->get_espnowid()));
                device_info->mutable_uart_info()->set_device_file(std::string(device->get_device_file()));
                const auto& details = device->get_device_details();
                device_info->set_device_name(details.device_name);
                if(details.espnow_version == start_device::espnow_version::V1) {
                    device_info->set_version(control_messages::basic_device_info::V1);
                } else {
                    device_info->set_version(control_messages::basic_device_info::V2);
                }
                auto* firmware_version = device_info->mutable_fw_version();
                firmware_version->set_major(details.firmware_version.major);
                firmware_version->set_minor(details.firmware_version.minor);
                firmware_version->set_patch(details.firmware_version.patch);
                device_info->set_mac_address(details.mac_address, 6);
            }

            response.SerializeToString(&response_);

            asio::async_write(socket_, asio::buffer(response_),
                std::bind(&control_session::handle_write, shared_from_this(),
                    asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        trigger_reading();
    }
}

void control_session::on_new_uart_device(espnow_uart_device* uart_device){
    control_messages::control_envelope response;
    auto* response_add = response.mutable_add_uart_device_response();
    response_add->set_device_id(std::string(uart_device->get_espnowid()));

    response.SerializeToString(&response_);

    asio::async_write(socket_, asio::buffer(response_),
        std::bind(&control_session::handle_write, shared_from_this(),
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}