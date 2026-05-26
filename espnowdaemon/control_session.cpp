#include "control_session.hpp"
#include <iostream>
#include <functional>
#include "control_messages.pb.h"

control_session::pointer control_session::create(asio::io_context &io_context, device_manager& device_manager) {
    return std::make_shared<control_session>(io_context, device_manager);
}

control_session::control_session(asio::io_context &io_context, device_manager& device_manager)
    : socket_(io_context), device_manager_(device_manager) {}

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

        //legacy handling
        //nlohmann::json json = nlohmann::json::parse(data_.data(), data_.data() + bytes_transferred);
        //if(json["action"] == "add_uart_device") {
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
            } else {
                response_add->set_status(control_messages::add_uart_device_response::SUCCESS);
                response_add->set_device_id(std::string(device.value()->get_espnowid()));
                std::cout << "Added " << request.device_file() << " with espnow id: " << device.value()->get_espnowid() << std::endl;
            }
            response.SerializeToString(&response_);

            asio::async_write(socket_, asio::buffer(response_),
                std::bind(&control_session::handle_write, shared_from_this(),
                    asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        trigger_reading();
    }
}
