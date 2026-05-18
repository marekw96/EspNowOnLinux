#include "espnow_uart_device.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "messages/utility.hpp"
#include "messages/message_id.hpp"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"
#include "messages/received_packet.hpp"
#include "messages/packet_to_send.hpp"
#include "messages/ping.hpp"

namespace {
    int open_tun_device(std::string_view interface_name) {
        auto m_fd = ::open("/dev/net/tun", O_RDWR);
        if (m_fd < 0) {
            throw std::runtime_error("Failed to open /dev/net/tun: " + std::string(strerror(errno)));
        }

        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));

        // Flags: IFF_TAP (Layer 2) | IFF_NO_PI (Do not provide packet information)
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

        if (!interface_name.empty()) {
            std::strncpy(ifr.ifr_name, interface_name.data(), IFNAMSIZ - 1);
            ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        }

        if (::ioctl(m_fd, TUNSETIFF, reinterpret_cast<void*>(&ifr)) < 0) {
            ::close(m_fd);
            throw std::runtime_error("Failed to ioctl TUNSETIFF on tap device " + std::string(interface_name) + ": " + std::string(strerror(errno)));
        }

        return m_fd;
    }
}

std::expected<espnow_uart_device::pointer, espnow_uart_device::opening_device_error> espnow_uart_device::open(asio::io_context& io_context, std::string_view device_file, uint32_t espnow_idx) {
    asio::serial_port serial_port(io_context);
    try{
        serial_port.open(std::string(device_file));
    } catch(asio::system_error& e) {
        std::cout << "no_such_device" << asio::error::no_such_device << std::endl;
        std::cout << "not_found" << asio::error::not_found << std::endl;

        if(e.code().value() == ENOENT) {
            return std::unexpected(opening_device_error::device_file_does_not_exist);
        } else {
            std::cerr << "Error opening uart device: " << e.code() << " " << e.what() << std::endl;
            return std::unexpected(opening_device_error::unknown_error);
        }
    }

    std::string espnow_id = "espnow" + std::to_string(espnow_idx);

    auto tun_fd = open_tun_device(espnow_id);
    asio::posix::stream_descriptor tun_fd_descriptor(io_context, tun_fd);

    return std::make_shared<espnow_uart_device>(std::move(serial_port), espnow_id, std::move(tun_fd_descriptor));
}

espnow_uart_device::espnow_uart_device(asio::serial_port serial_port, std::string espnow_id, asio::posix::stream_descriptor tun_fd)
    : serial_port_(std::move(serial_port)), espnow_id_(std::move(espnow_id)), tun_fd_(std::move(tun_fd)) {
        start_reading_serial_port();
        start_reading_tun();
}

std::string_view espnow_uart_device::get_espnowid() const {
    return espnow_id_;
}

asio::const_buffer espnow_uart_device::packet_buffer::as_buffer() const {
    return asio::buffer(buffer, size);
}

void espnow_uart_device::start_reading_tun() {
    tun_fd_.async_read_some(asio::buffer(tun_fd_buffer_),
        std::bind(&espnow_uart_device::tun_read_handle, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void espnow_uart_device::start_reading_serial_port() {
    auto available = serial_port_buffer_.get_writable();
    serial_port_.async_read_some(asio::buffer(available.data(), available.size()),
        std::bind(&espnow_uart_device::serial_port_read_handle, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void espnow_uart_device::tun_read_handle(const asio::error_code& ec, size_t bytes_transferred) {
    if(ec) {
        std::cerr << "Error reading tun: " << ec.message() << std::endl;
        return;
    }

    uint16_t ethertype = network_to_host(*reinterpret_cast<uint16_t*>(tun_fd_buffer_.data() + 12));
    if(ethertype == 0x88B5) {
        packet_to_send packet;
        memcpy(packet.destination_mac, tun_fd_buffer_.data(), 6);
        packet.data.insert(packet.data.end(), tun_fd_buffer_.data() + 14, tun_fd_buffer_.data() + bytes_transferred);

        packet_buffer buffer;
        auto serialized = io<packet_to_send>::serialize(packet);
        buffer.size = serialized.size();
        memcpy(buffer.buffer.data(), serialized.data(), serialized.size());
        serial_port_write_buffers_.push_back(buffer);
        std::cout << espnow_id_ << ": sending espnow packet size: " << packet.data.size() << std::endl;
        start_writing_serial_port();
    }

    start_reading_tun();
}

void espnow_uart_device::start_writing_tun() {
    if (tun_fd_write_buffers_.empty()) return;

    tun_fd_.async_write_some(tun_fd_write_buffers_.front().as_buffer(),
        std::bind(&espnow_uart_device::handle_tun_write, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void espnow_uart_device::handle_tun_write(const asio::error_code& ec, size_t bytes_transferred) {
    if(ec) {
        std::cerr << "Error writing tun: " << ec.message() << std::endl;
        return;
    }

    tun_fd_write_buffers_.pop_front();
    start_writing_tun();
}

void espnow_uart_device::start_writing_serial_port() {
    if (serial_port_write_buffers_.empty()) return;

    serial_port_.async_write_some(serial_port_write_buffers_.front().as_buffer(),
        std::bind(&espnow_uart_device::handle_serial_port_write, this,
            asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void espnow_uart_device::handle_serial_port_write(const asio::error_code& ec, size_t bytes_transferred) {
    if(ec) {
        std::cerr << "Error writing serial port: " << ec.message() << std::endl;
        return;
    }

    serial_port_write_buffers_.pop_front();
    start_writing_serial_port();
}

void espnow_uart_device::serial_port_read_handle(const asio::error_code& ec, size_t bytes_transferred) {
    if(ec) {
        std::cerr << "Error reading serial port: " << ec.message() << std::endl;
        return;
    }

    serial_port_buffer_.commit(bytes_transferred);

    auto data = serial_port_buffer_.get_data();
    while(!data.empty()) {
        auto processed_bytes = handle_serial_packet(data);
        if(processed_bytes == -1) {
            serial_port_buffer_.move_data_to_front();
            break;
        }

        if(processed_bytes == 0) {
            std::cerr << "Error handling serial packet" << std::endl;
            break;
        }

        serial_port_buffer_.consume(processed_bytes);
        data = serial_port_buffer_.get_data();
    }

    serial_port_buffer_.move_data_to_front();

    start_reading_serial_port();
}

int32_t espnow_uart_device::handle_serial_packet(std::span<uint8_t> data) {
    message_id id = static_cast<message_id>(data[0]);
    if(id == message_id::START_DEVICE) {
        if(data.size() >= sizeof(start_device)){
            if(memcmp(data.data() + 1, "espnowonlinux", 13) == 0) {
                std::cout << espnow_id_ << ": received start device message" << std::endl;
                if(!requested_start_) {
                    requested_start_ = true;
                    start_host message_to_send;
                    packet_buffer buffer;
                    memcpy(buffer.buffer.data(), &message_to_send, sizeof(message_to_send));
                    buffer.size = sizeof(message_to_send);
                    serial_port_write_buffers_.push_back(buffer);
                    start_writing_serial_port();
                }
            }
            return sizeof(start_device);
        }
    }
    else if (id == message_id::LOG_INFO){
        auto new_line_it = std::find(data.begin(), data.end(), '\n');
        if(new_line_it != data.end())
        {
            auto length = std::distance(data.begin(), new_line_it);
            data[length] = '\0';
            std::cout << espnow_id_ << ": [Info:" << length << "] " << data.data() + 1 << std::endl;
            return length + 1;
        }
        return -1;
    }
    else if(id == message_id::LOG_INFO_UART){
        if(data.size() < sizeof(message_id) + sizeof(uint32_t)) {
            std::cout << espnow_id_ << ": received log info header but not payload" << std::endl;
            return -1;
        }
        auto expected_size = host_to_network(*reinterpret_cast<uint32_t*>(data.data() + 1));
        if(data.size() >= sizeof(message_id) + sizeof(uint32_t) + expected_size)
        {
            std::cout << espnow_id_ << ": [InfoUart:" << expected_size << "] " << data.data() + 1 + sizeof(uint32_t) << std::endl;
            return sizeof(message_id) + sizeof(uint32_t) + expected_size;
        }
        return -1;
    }
    else if(id == message_id::PING){
        if(data.size() >= sizeof(ping)) {
            std::cout << espnow_id_ << ": received ping message" << std::endl;
            return sizeof(ping);
        }
        return -1;
    }
    else if(id == message_id::RECEIVED_PACKET){
        if(data.size() < sizeof(message_id) + sizeof(uint32_t)) {
            std::cout << espnow_id_ << ": received packet header but not payload" << std::endl;
            return -1;
        }
        auto sub_buffer = data.subspan(1);
        auto size_buffer = sub_buffer.subspan(6,4);
        auto payload_buffer = sub_buffer.subspan(10);
        auto expected_size = network_to_host(*reinterpret_cast<uint32_t*>(size_buffer.data()));
        if(expected_size < payload_buffer.size()) {
            std::cout << espnow_id_ << " : received packet but it is incomplete" << std::endl;
            return -1;
        }

        auto message = io<received_packet>::deserialize(sub_buffer);
        std::cout << espnow_id_ << ": received packet payload size: " << message.data.size() << std::endl;

        packet_buffer buffer;
        uint8_t dest_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        uint8_t ethertype[] = {0x88, 0xb5};
        std::memcpy(buffer.buffer.data(), dest_mac, 6);
        std::memcpy(buffer.buffer.data() + 6, message.mac, 6);
        std::memcpy(buffer.buffer.data() + 12, ethertype, 2);
        std::memcpy(buffer.buffer.data() + 14, message.data.data(), message.data.size());
        buffer.size = 14 + message.data.size();
        tun_fd_write_buffers_.push_back(buffer);
        start_writing_tun();

        return sizeof(message_id) + 6 + sizeof(uint32_t) + payload_buffer.size();
    }
    else {
        auto print_bytes = [this](std::span<uint8_t> span){
            std::cout << espnow_id_ << ": ";
            for(auto x : span) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)x << " ";
            }
            std::cout << " | ";
            for(auto x : span) {
                if(std::isalnum(x)) {
                    std::cout << (char)x;
                }
                else {
                    std::cout << ".";
                }
            }
            std::cout << std::dec << std::endl;
        };
        auto off = 0u;
        while(off < data.size()) {
            auto sub = data.subspan(off, 32);
            print_bytes(sub);
            off += 32;
        }

        return data.size();
    }

    return -1;
}
