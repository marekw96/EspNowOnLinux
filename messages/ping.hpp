#pragma once

#include "message_id.hpp"

struct ping {
    message_id id = message_id::PING;
    char i = 'i';
    char n = 'n';
    char g = 'g';
}__attribute__((packed));