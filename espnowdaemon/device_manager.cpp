#include "device_manager.hpp"

device_manager::device_manager(asio::io_context& io_context) : io_context_(io_context) {}

std::expected<espnow_uart_device*, adding_device_error_code> device_manager::add_uart_device(std::string_view device_file) {
    auto device = espnow_uart_device::open(io_context_, device_file, espnow_devices_.size());
    if (!device) {
        return std::unexpected(adding_device_error_code::device_file_does_not_exist);
    }

    espnow_devices_.push_back(device.value());
    return espnow_devices_.back().get();
}

std::optional<std::reference_wrapper<espnow_uart_device>> device_manager::get_device_by_id(std::string_view device_id) {
    for(auto& device : espnow_devices_){
        if(device->get_espnowid() == device_id){
            return std::ref(*device.get());
        }
    }
    return std::nullopt;
}