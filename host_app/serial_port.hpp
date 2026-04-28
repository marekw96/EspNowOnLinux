#pragma once

#include <string_view>
#include <span>
#include <cstddef>

class serial_port {
public:
    explicit serial_port(std::string_view port_name);
    ~serial_port();

    // Disable copy
    serial_port(const serial_port&) = delete;
    serial_port& operator=(const serial_port&) = delete;

    // Allow move
    serial_port(serial_port&& other) noexcept;
    serial_port& operator=(serial_port&& other) noexcept;

    int read(std::span<unsigned char> buffer);
    int write(std::span<const unsigned char> buffer);

    int get_fd() const;

private:
    int m_fd = -1;
};
