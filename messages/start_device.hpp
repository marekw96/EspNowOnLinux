#pragma once

#include "message_id.hpp"
#include "utility.hpp"

struct start_device {
    enum class espnow_version : uint8_t {
        V1 = 1,
        V2 = 2
    };

    struct firmware_version {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
    };

    message_id id = message_id::START_DEVICE;
    char header[13] = {'e', 's', 'p', 'n', 'o', 'w', 'o', 'n', 'l', 'i', 'n', 'u', 'x'};
    char device_name[10] = {0};
    espnow_version version = espnow_version::V1;
    firmware_version fw_version = {0, 0, 0};
    uint8_t mac_address[6] = {0};
};

template<>
struct buffer_utils<start_device::espnow_version> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, start_device::espnow_version& value) {
        return buffer_utils<uint8_t>::read(buffer, reinterpret_cast<uint8_t&>(value));
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, start_device::espnow_version value) {
        return buffer_utils<uint8_t>::write(buffer, static_cast<uint8_t>(value));
    }
};

template<>
struct buffer_utils<start_device::firmware_version> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, start_device::firmware_version& value) {
        assert(buffer.size() >= sizeof(start_device::firmware_version));

        buffer = buffer_utils<uint8_t>::read(buffer, value.major);
        buffer = buffer_utils<uint8_t>::read(buffer, value.minor);
        buffer = buffer_utils<uint8_t>::read(buffer, value.patch);

        return buffer;
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, const start_device::firmware_version& value) {
        assert(buffer.size() >= sizeof(start_device::firmware_version));

        buffer = buffer_utils<uint8_t>::write(buffer, value.major);
        buffer = buffer_utils<uint8_t>::write(buffer, value.minor);
        buffer = buffer_utils<uint8_t>::write(buffer, value.patch);

        return buffer;
    }
};

template<>
struct buffer_utils<start_device> {
    static inline std::span<const uint8_t> read(std::span<const uint8_t> buffer, start_device& value) {
        assert(buffer.size() >= sizeof(start_device));

        buffer = buffer_utils<message_id>::read(buffer, value.id);
        buffer = buffer_utils<char[13]>::read(buffer, value.header);
        buffer = buffer_utils<char[10]>::read(buffer, value.device_name);
        buffer = buffer_utils<start_device::espnow_version>::read(buffer, value.version);
        buffer = buffer_utils<start_device::firmware_version>::read(buffer, value.fw_version);
        buffer = buffer_utils<uint8_t[6]>::read(buffer, value.mac_address);

        return buffer;
    }

    static inline std::span<uint8_t> write(std::span<uint8_t> buffer, const start_device& value) {
        assert(buffer.size() >= sizeof(start_device));

        buffer = buffer_utils<message_id>::write(buffer, value.id);
        buffer = buffer_utils<char[13]>::write(buffer, value.header);
        buffer = buffer_utils<char[10]>::write(buffer, value.device_name);
        buffer = buffer_utils<start_device::espnow_version>::write(buffer, value.version);
        buffer = buffer_utils<start_device::firmware_version>::write(buffer, value.fw_version);
        buffer = buffer_utils<uint8_t[6]>::write(buffer, value.mac_address);

        return buffer;
    }
};