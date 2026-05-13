#pragma once

#include "control_session.hpp"
#include "device_manager.hpp"
#include <asio.hpp>

class control_server {
public:
    control_server(asio::io_context &io_context, device_manager& device_manager);

    void start();

private:
    void handle_accept(control_session::pointer session, std::error_code ec);

    asio::io_context &io_context_;
    asio::ip::tcp::acceptor acceptor_;
    device_manager& device_manager_;
};
