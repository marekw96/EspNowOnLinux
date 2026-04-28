#include "tap_device.hpp"

#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

tap_device::tap_device(std::string_view interface_name) : m_name(interface_name) {
    m_fd = ::open("/dev/net/tun", O_RDWR);
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
        m_fd = -1;
        throw std::runtime_error("Failed to ioctl TUNSETIFF on tap device " + m_name + ": " + std::string(strerror(errno)));
    }

    // Update name with the actual interface name in case it was dynamically allocated
    m_name = ifr.ifr_name;
}

tap_device::~tap_device() {
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

tap_device::tap_device(tap_device&& other) noexcept : m_fd(other.m_fd), m_name(std::move(other.m_name)) {
    other.m_fd = -1;
}

tap_device& tap_device::operator=(tap_device&& other) noexcept {
    if (this != &other) {
        if (m_fd != -1) {
            ::close(m_fd);
        }
        m_fd = other.m_fd;
        m_name = std::move(other.m_name);
        other.m_fd = -1;
    }
    return *this;
}

int tap_device::read(std::span<unsigned char> buffer) {
    if (m_fd == -1) return -1;
    return ::read(m_fd, buffer.data(), buffer.size());
}

int tap_device::write(std::span<const unsigned char> buffer) {
    if (m_fd == -1) return -1;
    return ::write(m_fd, buffer.data(), buffer.size());
}

std::string tap_device::get_name() const {
    return m_name;
}

int tap_device::get_fd() const {
    return m_fd;
}
