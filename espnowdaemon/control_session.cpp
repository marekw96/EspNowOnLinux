#include "control_session.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <functional>

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
        nlohmann::json json = nlohmann::json::parse(data_.data(), data_.data() + bytes_transferred);
        if(json["action"] == "add_uart_device") {
            std::cout << "Adding uart device: " << json["device_file"] << std::endl;
            auto device = device_manager_.add_uart_device(json["device_file"].get<std::string>());
            response_ = "{\"action_status\" : \"ok\"}";

            if (!device) {
                std::cerr << "Error adding uart device: " << static_cast<int>(device.error()) << std::endl;
                if (device.error() == adding_device_error_code::device_file_does_not_exist) {
                    response_ = "{\"action_status\" : \"device_file_does_not_exist\"}";
                } else {
                    response_ = "{\"action_status\" : \"unknown_error\"}";
                }
            } else {
                response_ = "{\"action_status\" : \"ok\", \"espnow_id\" : \"" + std::string(device.value()->get_espnowid()) + "\"}";
                std::cout << "Added " << json["device_file"] << " with espnow id: " << device.value()->get_espnowid() << std::endl;
            }

            std::cout << "Response: " << response_ << std::endl;

            asio::async_write(socket_, asio::buffer(response_),
                std::bind(&control_session::handle_write, shared_from_this(),
                    asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        trigger_reading();
    }
}
