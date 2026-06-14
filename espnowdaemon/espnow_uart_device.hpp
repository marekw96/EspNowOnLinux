#pragma once

#include <asio.hpp>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <deque>
#include <utility/receiving_buffer.hpp>
#include <chrono>
#include "messages/start_device.hpp"
#include "serial_port_socket.hpp"

using uptime_t = std::chrono::seconds;

struct statistics {
    uint64_t broadcast_sent = 0;
    uint64_t broadcast_received = 0;
};

struct device_details {
    std::string device_name;
    uint8_t mac_address[6];
    start_device::espnow_version espnow_version;
    start_device::firmware_version firmware_version;
};

class espnow_uart_device : public std::enable_shared_from_this<espnow_uart_device> {
public:
    using pointer = std::shared_ptr<espnow_uart_device>;

    enum class opening_device_error{
        ok = 0,
        device_file_does_not_exist,
        device_already_opened,
        unknown_error
    };

    static std::expected<pointer, opening_device_error> open(asio::io_context& io_context, std::string_view device_file, uint32_t espnow_idx);

    espnow_uart_device(std::unique_ptr<serial_port_socket> serial_port, std::string espnow_id, asio::posix::stream_descriptor tun_fd, std::string_view device_file);

    std::string_view get_espnowid() const;
    std::string_view get_device_file() const;
    statistics get_statistics() const;
    uptime_t get_uptime() const;
    const device_details& get_device_details() const;

private:
    struct packet_buffer {
        std::array<uint8_t, 512> buffer;
        size_t size;

        asio::const_buffer as_buffer() const;
    };

    bool requested_start_ = false;
    device_details device_details_;
    std::string espnow_id_;
    std::string device_file_;
    std::unique_ptr<serial_port_socket> serial_port_;
    asio::posix::stream_descriptor tun_fd_;
    std::array<uint8_t, 512> tun_fd_buffer_;
    std::deque<packet_buffer> tun_fd_write_buffers_;

    statistics statistics_;
    std::chrono::steady_clock::time_point boot_time_;

    void start_reading_tun();
    void tun_read_handle(const asio::error_code& ec, size_t bytes_transferred);
    void start_writing_tun();
    void handle_tun_write(const asio::error_code& ec, size_t bytes_transferred);
    int32_t serial_port_read_handle(const asio::error_code& ec, std::span<uint8_t> data);
    int32_t handle_serial_packet(std::span<uint8_t> data);
};
