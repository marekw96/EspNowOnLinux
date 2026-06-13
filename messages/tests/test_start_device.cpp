#include <gtest/gtest.h>
#include "start_device.hpp"
#include <array>
#include <cstring>

// Test start_device::espnow_version specialization
TEST(StartDeviceTest, EspnowVersionSerialization) {
    start_device::espnow_version original = start_device::espnow_version::V2;
    std::array<uint8_t, 2> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<start_device::espnow_version>::write(buffer, original);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(start_device::espnow_version::V2));
    EXPECT_EQ(write_span.size(), 1);

    // Read from buffer
    start_device::espnow_version deserialized = start_device::espnow_version::V1;
    auto read_span = buffer_utils<start_device::espnow_version>::read(buffer, deserialized);
    EXPECT_EQ(deserialized, original);
    EXPECT_EQ(read_span.size(), 1);
}

// Test start_device::firmware_version specialization
TEST(StartDeviceTest, FirmwareVersionSerialization) {
    start_device::firmware_version original{2, 4, 1};
    std::array<uint8_t, 5> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<start_device::firmware_version>::write(buffer, original);
    EXPECT_EQ(buffer[0], 2);
    EXPECT_EQ(buffer[1], 4);
    EXPECT_EQ(buffer[2], 1);
    EXPECT_EQ(write_span.size(), 2);

    // Read from buffer
    start_device::firmware_version deserialized{0, 0, 0};
    auto read_span = buffer_utils<start_device::firmware_version>::read(buffer, deserialized);
    EXPECT_EQ(deserialized.major, original.major);
    EXPECT_EQ(deserialized.minor, original.minor);
    EXPECT_EQ(deserialized.patch, original.patch);
    EXPECT_EQ(read_span.size(), 2);
}

// Test start_device specialization
TEST(StartDeviceTest, StartDeviceSerialization) {
    start_device original;
    original.id = message_id::START_DEVICE;
    std::memcpy(original.device_name, "my-device", 9);
    original.device_name[9] = '\0';
    original.version = start_device::espnow_version::V2;
    original.fw_version = {1, 2, 3};
    original.mac_address[0] = 0x11;
    original.mac_address[1] = 0x22;
    original.mac_address[2] = 0x33;
    original.mac_address[3] = 0x44;
    original.mac_address[4] = 0x55;
    original.mac_address[5] = 0x66;

    std::array<uint8_t, start_device::SIZE + 5> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<start_device>::write(buffer, original);
    EXPECT_EQ(write_span.size(), 5);

    // Read from buffer
    start_device deserialized;
    auto read_span = buffer_utils<start_device>::read(buffer, deserialized);

    EXPECT_EQ(deserialized.id, original.id);
    EXPECT_EQ(deserialized.id, message_id::START_DEVICE);
    for (size_t i = 0; i < 13; ++i) {
        EXPECT_EQ(deserialized.header[i], original.header[i]);
    }
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(deserialized.device_name[i], original.device_name[i]);
    }
    EXPECT_EQ(deserialized.version, original.version);
    EXPECT_EQ(deserialized.fw_version.major, original.fw_version.major);
    EXPECT_EQ(deserialized.fw_version.minor, original.fw_version.minor);
    EXPECT_EQ(deserialized.fw_version.patch, original.fw_version.patch);
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(deserialized.mac_address[i], original.mac_address[i]);
    }
    EXPECT_EQ(read_span.size(), 5);
}
