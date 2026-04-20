#pragma once

#include <string>
#include <string_view>
#include <span>
#include <cstddef>

class tap_device {
public:
    explicit tap_device(std::string_view interface_name);
    ~tap_device();

    // Disable copy
    tap_device(const tap_device&) = delete;
    tap_device& operator=(const tap_device&) = delete;

    // Allow move
    tap_device(tap_device&& other) noexcept;
    tap_device& operator=(tap_device&& other) noexcept;

    int read(std::span<unsigned char> buffer);
    int write(std::span<const unsigned char> buffer);

    std::string get_name() const;

private:
    int m_fd = -1;
    std::string m_name;
};
