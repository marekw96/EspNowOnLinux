#pragma once

#include <asio.hpp>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <deque>
#include <utility/receiving_buffer.hpp>

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

    espnow_uart_device(asio::serial_port serial_port, std::string espnow_id, asio::posix::stream_descriptor tun_fd);

    std::string_view get_espnowid() const;

private:
    struct packet_buffer {
        std::array<uint8_t, 512> buffer;
        size_t size;

        asio::const_buffer as_buffer() const;
    };

    bool requested_start_ = false;
    std::string espnow_id_;
    asio::serial_port serial_port_;
    asio::posix::stream_descriptor tun_fd_;
    receiving_buffer<1024 * 4> serial_port_buffer_;
    std::deque<packet_buffer> serial_port_write_buffers_;
    std::array<uint8_t, 512> tun_fd_buffer_;
    std::deque<packet_buffer> tun_fd_write_buffers_;

    void start_reading_tun();
    void start_reading_serial_port();
    void tun_read_handle(const asio::error_code& ec, size_t bytes_transferred);
    void start_writing_tun();
    void handle_tun_write(const asio::error_code& ec, size_t bytes_transferred);
    void start_writing_serial_port();
    void handle_serial_port_write(const asio::error_code& ec, size_t bytes_transferred);
    void serial_port_read_handle(const asio::error_code& ec, size_t bytes_transferred);
    int32_t handle_serial_packet(std::span<uint8_t> data);
};
