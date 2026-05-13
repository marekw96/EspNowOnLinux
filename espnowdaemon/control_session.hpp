#pragma once

#include "device_manager.hpp"
#include <asio.hpp>
#include <memory>
#include <array>
#include <string>

class control_session : public std::enable_shared_from_this<control_session> {
public:
    using pointer = std::shared_ptr<control_session>;

    static pointer create(asio::io_context &io_context, device_manager& device_manager);

    control_session(asio::io_context &io_context, device_manager& device_manager);

    asio::ip::tcp::socket &get_socket();

    void start();

private:
    void trigger_reading();
    void handle_write(const std::error_code& ec, size_t bytes_transferred);
    void handle_read(const std::error_code& ec, size_t bytes_transferred);

    asio::ip::tcp::socket socket_;
    device_manager& device_manager_;
    std::array<char, 4 * 1024> data_;
    std::string response_;
};
