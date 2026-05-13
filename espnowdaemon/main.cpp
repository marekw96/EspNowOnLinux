#include <asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>
#include <expected>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <deque>

#include "messages/utility.hpp"
#include "messages/message_id.hpp"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"
#include "messages/received_packet.hpp"
#include "messages/packet_to_send.hpp"

using namespace asio;
using asio::ip::tcp;

constexpr auto CONTROL_PORT = 19997;

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

class espnow_uart_device : public std::enable_shared_from_this<espnow_uart_device> {
public:
    using pointer = std::shared_ptr<espnow_uart_device>;

    enum class opening_device_error{
        ok = 0,
        device_file_does_not_exist,
        device_already_opened,
        unknown_error
    };

    static std::expected<pointer, opening_device_error> open(asio::io_context& io_context, std::string_view device_file, uint32_t espnow_idx) {
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

    espnow_uart_device(asio::serial_port serial_port, std::string espnow_id, asio::posix::stream_descriptor tun_fd)
        : serial_port_(std::move(serial_port)), espnow_id_(std::move(espnow_id)), tun_fd_(std::move(tun_fd)) {
            start_reading_serial_port();
            start_reading_tun();
    }

    std::string_view get_espnowid() const {
        return espnow_id_;
    }

private:
    struct packet_buffer {
        std::array<uint8_t, 512> buffer;
        size_t size;

        asio::const_buffer as_buffer() const {
            return asio::buffer(buffer, size);
        }
    };

    std::string espnow_id_;
    asio::serial_port serial_port_;
    asio::posix::stream_descriptor tun_fd_;
    std::array<uint8_t, 512> serial_port_buffer_;
    std::deque<packet_buffer> serial_port_write_buffers_;
    std::array<uint8_t, 512> tun_fd_buffer_;
    std::deque<packet_buffer> tun_fd_write_buffers_;

    void start_reading_tun() {
        tun_fd_.async_read_some(asio::buffer(tun_fd_buffer_),
            std::bind(&espnow_uart_device::tun_read_handle, this,
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void start_reading_serial_port() {
        serial_port_.async_read_some(asio::buffer(serial_port_buffer_),
            std::bind(&espnow_uart_device::serial_port_read_handle, this,
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void tun_read_handle(const asio::error_code& ec, size_t bytes_transferred) {
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
            start_writing_serial_port();
        }

        start_reading_tun();
    }

    void start_writing_tun() {
        if (tun_fd_write_buffers_.empty()) return;

        tun_fd_.async_write_some(tun_fd_write_buffers_.front().as_buffer(),
            std::bind(&espnow_uart_device::handle_tun_write, this,
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void handle_tun_write(const asio::error_code& ec, size_t bytes_transferred) {
        if(ec) {
            std::cerr << "Error writing tun: " << ec.message() << std::endl;
            return;
        }

        tun_fd_write_buffers_.pop_front();
        start_writing_tun();
    }

    void start_writing_serial_port() {
        if (serial_port_write_buffers_.empty()) return;

        serial_port_.async_write_some(serial_port_write_buffers_.front().as_buffer(),
            std::bind(&espnow_uart_device::handle_serial_port_write, this,
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void handle_serial_port_write(const asio::error_code& ec, size_t bytes_transferred) {
        if(ec) {
            std::cerr << "Error writing serial port: " << ec.message() << std::endl;
            return;
        }

        serial_port_write_buffers_.pop_front();
        start_writing_serial_port();
    }

    void serial_port_read_handle(const asio::error_code& ec, size_t bytes_transferred) {
        if(ec) {
            std::cerr << "Error reading serial port: " << ec.message() << std::endl;
            return;
        }

        message_id id = static_cast<message_id>(serial_port_buffer_[0]);
        if(id == message_id::START_DEVICE) {
            if(bytes_transferred == sizeof(start_device)){
                if(memcmp(serial_port_buffer_.data() + 1, "espnowonlinux", 13) == 0) {
                    std::cout << espnow_id_ << ": received start device message" << std::endl;
                    start_host message_to_send;
                    packet_buffer buffer;
                    memcpy(buffer.buffer.data(), &message_to_send, sizeof(message_to_send));
                    buffer.size = sizeof(message_to_send);
                    serial_port_write_buffers_.push_back(buffer);
                    start_writing_serial_port();
                }
            }
        }
        else if (id == message_id::LOG_INFO){
            auto* message = reinterpret_cast<char*>(serial_port_buffer_.data());
            message[bytes_transferred-1] = '\0';
            std::cout << espnow_id_ << ": [Info] " << message + 1 << std::endl;
        }
        else if(id == message_id::RECEIVED_PACKET){
            auto message = io<received_packet>::deserialize(std::span<const unsigned char>(serial_port_buffer_.data(), bytes_transferred));
            std::cout << espnow_id_ << ": received packet" << std::endl;

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
        }

        start_reading_serial_port();
    }
};

enum class adding_device_error_code {
    ok = 0,
    device_already_exists,
    device_file_does_not_exist,
};

class device_manager {
public:
    device_manager(asio::io_context& io_context) : io_context_(io_context) {}

    std::expected<espnow_uart_device*, adding_device_error_code> add_uart_device(std::string_view device_file) {
        auto device = espnow_uart_device::open(io_context_, device_file, espnow_devices_.size());
        if (!device) {
            return std::unexpected(adding_device_error_code::device_file_does_not_exist);
        }

        espnow_devices_.push_back(device.value());
        return espnow_devices_.back().get();
    }

private:
    std::vector<espnow_uart_device::pointer> espnow_devices_;
    asio::io_context& io_context_;
};

class control_session
    : public std::enable_shared_from_this<control_session>
{
public:
    using pointer = std::shared_ptr<control_session>;

    static pointer create(asio::io_context &io_context, device_manager& device_manager) {
        return std::make_shared<control_session>(io_context, device_manager);
    }

    tcp::socket &get_socket() { return socket_; }

    void start() {
        std::cout << "New control session started" << std::endl;
        trigger_reading();
    }

    control_session(asio::io_context &io_context, device_manager& device_manager)
    : socket_(io_context), device_manager_(device_manager) {}

private:
    void trigger_reading(){
        socket_.async_read_some(asio::buffer(data_),
            std::bind(&control_session::handle_read, shared_from_this(),
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    void handle_write(const std::error_code& ec, size_t bytes_transferred)
    {}

    void handle_read(const std::error_code& ec, size_t bytes_transferred)
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

    asio::ip::tcp::socket socket_;
    device_manager& device_manager_;
    std::array<char, 4 * 1024> data_;
    std::string response_;
};

class control_server
{
public:
    control_server(asio::io_context &io_context, device_manager& device_manager)
    : io_context_(io_context)
    , device_manager_(device_manager)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), CONTROL_PORT)) {
        start();
    }

    void start() {
        auto session = control_session::create(io_context_, device_manager_);

        acceptor_.async_accept(
            session->get_socket(),
            std::bind(&control_server::handle_accept, this, session, asio::placeholders::error));
    }

    void handle_accept(control_session::pointer session, std::error_code ec)
    {
        if (!ec) {
            session->start();
        }

        start();
    }

private:
    asio::io_context &io_context_;
    tcp::acceptor acceptor_;
    device_manager& device_manager_;
};

int main(int argc, char *argv[])
{
    // try {
        asio::io_context io_context(1);

        device_manager device_manager(io_context);
        control_server server(io_context, device_manager);

        io_context.run();
    // }
    // catch (std::exception &e) {
    //     std::cerr << "Exception: " << e.what() << std::endl;
    //     return 1;
    // }

    return 0;
}