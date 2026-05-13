#pragma once

#include "espnow_uart_device.hpp"
#include <asio.hpp>
#include <expected>
#include <string_view>
#include <vector>

enum class adding_device_error_code {
    ok = 0,
    device_already_exists,
    device_file_does_not_exist,
};

class device_manager {
public:
    device_manager(asio::io_context& io_context);

    std::expected<espnow_uart_device*, adding_device_error_code> add_uart_device(std::string_view device_file);

private:
    std::vector<espnow_uart_device::pointer> espnow_devices_;
    asio::io_context& io_context_;
};
