#pragma once

enum class message_id : unsigned char {
    START_DEVICE = 0,
    START_HOST,
    LOG_INFO = 'I',
};