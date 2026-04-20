#include "serial_port.hpp"
#include <iostream>
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <cerrno>    // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <cstring>
#include <stdexcept>

serial_port::serial_port(std::string_view port_name) {
    m_fd = ::open(std::string(port_name).c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    if (m_fd < 0) {
        throw std::runtime_error("Error " + std::to_string(errno) + " from open: " + strerror(errno));
    }

    struct termios tty;

    if(tcgetattr(m_fd, &tty) != 0) {
        ::close(m_fd);
        m_fd = -1;
        throw std::runtime_error("Error " + std::to_string(errno) + " from tcgetattr: " + strerror(errno));
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        ::close(m_fd);
        m_fd = -1;
        throw std::runtime_error("Error " + std::to_string(errno) + " from tcsetattr: " + strerror(errno));
    }
}

serial_port::~serial_port() {
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

serial_port::serial_port(serial_port&& other) noexcept : m_fd(other.m_fd) {
    other.m_fd = -1;
}

serial_port& serial_port::operator=(serial_port&& other) noexcept {
    if (this != &other) {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        m_fd = other.m_fd;
        other.m_fd = -1;
    }
    return *this;
}

int serial_port::read(std::span<unsigned char> buffer) {
    if (m_fd < 0) return -1;
    return ::read(m_fd, buffer.data(), buffer.size());
}

int serial_port::write(std::span<const unsigned char> buffer) {
    if (m_fd < 0) return -1;
    return ::write(m_fd, buffer.data(), buffer.size());
}
