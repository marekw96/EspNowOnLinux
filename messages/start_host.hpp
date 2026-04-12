#pragma once

#include "message_id.hpp"

struct start_host {
    message_id id = message_id::START_HOST;
} __attribute__((packed));
