#pragma once

#include "message_id.hpp"

enum class device_type : unsigned char {
    UNKNOWN = 0,
    ESP32_C3 = 1,
};

struct start_device {
    message_id id = message_id::START_DEVICE;
    char header[13] = {'e', 's', 'p', 'n', 'o', 'w', 'o', 'n', 'l', 'i', 'n', 'u', 'x'};
    device_type type;
} __attribute__((packed));