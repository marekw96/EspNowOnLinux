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

class espnow_device {
public:
    enum class opening_device_error{
        ok = 0,
        device_file_does_not_exist,
        device_already_opened,
        unknown_error
    };

    static std::expected<espnow_device, opening_device_error> open(asio::io_context& io_context, std::string_view device_file, uint32_t espnow_idx) {
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
        std::cout << "here" << std::endl;

        return espnow_device(std::move(serial_port), espnow_id, std::move(tun_fd_descriptor));
    }

    espnow_device(asio::serial_port serial_port, std::string espnow_id, asio::posix::stream_descriptor tun_fd)
        : serial_port_(std::move(serial_port)), espnow_id_(std::move(espnow_id)), tun_fd_(std::move(tun_fd)) {
        std::cout << "here4" << std::endl;
    }

    std::string_view get_espnowid() const {
        return espnow_id_;
    }

private:
    std::string espnow_id_;
    asio::serial_port serial_port_;
    asio::posix::stream_descriptor tun_fd_;
};

enum class adding_device_error_code {
    ok = 0,
    device_already_exists,
    device_file_does_not_exist,
};

class device_manager {
public:
    device_manager(asio::io_context& io_context) : io_context_(io_context) {}

    std::expected<espnow_device*, adding_device_error_code> add_uart_device(std::string_view device_file) {
        auto device = espnow_device::open(io_context_, device_file, espnow_devices_.size());
        if (!device) {
            return std::unexpected(adding_device_error_code::device_file_does_not_exist);
        }

        std::cout << "here2" << std::endl;

        espnow_devices_.push_back(std::move(device.value()));
        std::cout << "here3" << std::endl;
        return &espnow_devices_.back();
    }

    std::optional<espnow_device*> find_device_by_espnowid(std::string_view espnow_id) {
        for(auto& device : espnow_devices_) {
            if(device.get_espnowid() == espnow_id) {
                return &device;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<espnow_device> espnow_devices_;
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
                    // response = "{\"action_status\" : \"ok\", \"espnow_id\" : \"" + std::string(device.value()->get_espnowid()) + "\"}";
                    // std::cout << "Added " << json["device_file"] << " with espnow id: " << device.value()->get_espnowid() << std::endl;
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
    try {
        asio::io_context io_context(1);

        device_manager device_manager(io_context);
        control_server server(io_context, device_manager);

        io_context.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}