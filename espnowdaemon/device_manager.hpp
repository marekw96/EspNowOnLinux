#pragma once

#include "espnow_uart_device.hpp"
#include <asio.hpp>
#include <expected>
#include <optional>
#include <string_view>
#include <vector>
#include <functional>
#include "utility/event_dispatcher.hpp"

enum class adding_device_error_code {
    ok = 0,
    device_already_exists,
    device_file_does_not_exist,
};

class device_manager {
public:
    device_manager(asio::io_context& io_context, event_dispatcher& dispatcher);

    std::expected<espnow_uart_device*, adding_device_error_code> add_uart_device(std::string_view device_file);
    std::optional<std::reference_wrapper<espnow_uart_device>> get_device_by_id(std::string_view device_id);

    std::span<const espnow_uart_device::pointer> get_devices() const;

private:
    std::vector<espnow_uart_device::pointer> espnow_devices_;
    asio::io_context& io_context_;
    event_dispatcher& dispatcher_;
};
