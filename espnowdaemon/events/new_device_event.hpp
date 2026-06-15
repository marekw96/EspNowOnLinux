#pragma once

#include "espnow_uart_device.hpp"

struct new_device_event {
    using argument_type = espnow_uart_device*;
};