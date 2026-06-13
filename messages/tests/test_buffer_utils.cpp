#include <gtest/gtest.h>
#include "utility.hpp"
#include <array>
#include <vector>

// 1. Test uint8_t specialization
TEST(BufferUtilsTest, Uint8Serialization) {
    uint8_t original = 0xAA;
    std::array<uint8_t, 4> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<uint8_t>::write(buffer, original);
    EXPECT_EQ(buffer[0], 0xAA);
    EXPECT_EQ(write_span.size(), 3); // 3 bytes remaining in the buffer span

    // Read from buffer
    uint8_t deserialized = 0;
    auto read_span = buffer_utils<uint8_t>::read(buffer, deserialized);
    EXPECT_EQ(deserialized, original);
    EXPECT_EQ(read_span.size(), 3); // 3 bytes remaining in the buffer span
}

// 2. Test char specialization
TEST(BufferUtilsTest, CharSerialization) {
    char original = 'Z';
    std::array<uint8_t, 4> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<char>::write(buffer, original);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>('Z'));
    EXPECT_EQ(write_span.size(), 3);

    // Read from buffer
    char deserialized = '\0';
    auto read_span = buffer_utils<char>::read(buffer, deserialized);
    EXPECT_EQ(deserialized, original);
    EXPECT_EQ(read_span.size(), 3);
}

// 3. Test char[S] (fixed-size character array) specialization
TEST(BufferUtilsTest, CharArraySerialization) {
    char original[6] = "Hello"; // Includes null terminator
    std::array<uint8_t, 10> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<char[6]>::write(buffer, original);
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(buffer[i], static_cast<uint8_t>(original[i]));
    }
    EXPECT_EQ(write_span.size(), 4);

    // Read from buffer
    char deserialized[6] = {0};
    auto read_span = buffer_utils<char[6]>::read(buffer, deserialized);
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(deserialized[i], original[i]);
    }
    EXPECT_EQ(read_span.size(), 4);
}

// 4. Test uint8_t[S] (fixed-size byte array) specialization
TEST(BufferUtilsTest, Uint8ArraySerialization) {
    uint8_t original[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::array<uint8_t, 8> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<uint8_t[5]>::write(buffer, original);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(buffer[i], original[i]);
    }
    EXPECT_EQ(write_span.size(), 3);

    // Read from buffer
    uint8_t deserialized[5] = {0};
    auto read_span = buffer_utils<uint8_t[5]>::read(buffer, deserialized);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(deserialized[i], original[i]);
    }
    EXPECT_EQ(read_span.size(), 3);
}

// 5. Test uint16_t (big-endian/network order conversion) specialization
TEST(BufferUtilsTest, Uint16Serialization) {
    uint16_t original = 0xABCD;
    std::array<uint8_t, 4> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<uint16_t>::write(buffer, original);
    EXPECT_EQ(write_span.size(), 2);

    // Verify byte order (it should match the behavior of host_to_network followed by big-endian write)
    // On little endian machines (like x86_64 and ESP32), host_to_network (htons) swaps 0xABCD to 0xCDAB.
    // The write implementation then writes 0xCDAB as big-endian (MSB first, i.e., 0xCD then 0xAB).
    // Therefore, buffer[0] is 0xCD and buffer[1] is 0xAB.
    // Let's verify that this round-trips correctly first.
    uint16_t deserialized = 0;
    auto read_span = buffer_utils<uint16_t>::read(buffer, deserialized);
    EXPECT_EQ(deserialized, original);
    EXPECT_EQ(read_span.size(), 2);

    // Let's also assert the exact expected byte representation depending on machine endianness
    uint16_t expected_net_val = htons(original);
    uint8_t expected_byte0 = (expected_net_val >> 8) & 0xFF;
    uint8_t expected_byte1 = expected_net_val & 0xFF;
    EXPECT_EQ(buffer[0], expected_byte0);
    EXPECT_EQ(buffer[1], expected_byte1);
}

// 6. Test uint32_t (big-endian/network order conversion) specialization
TEST(BufferUtilsTest, Uint32Serialization) {
    uint32_t original = 0x12345678;
    std::array<uint8_t, 6> buffer{0};

    // Write to buffer
    auto write_span = buffer_utils<uint32_t>::write(buffer, original);
    EXPECT_EQ(write_span.size(), 2);

    // Verify that the round-trip deserialization matches the original value
    uint32_t deserialized = 0;
    auto read_span = buffer_utils<uint32_t>::read(buffer, deserialized);
    EXPECT_EQ(deserialized, original);
    EXPECT_EQ(read_span.size(), 2);

    // Let's assert the exact expected byte representation depending on machine endianness
    uint32_t expected_net_val = htonl(original);
    uint8_t expected_byte0 = (expected_net_val >> 24) & 0xFF;
    uint8_t expected_byte1 = (expected_net_val >> 16) & 0xFF;
    uint8_t expected_byte2 = (expected_net_val >> 8) & 0xFF;
    uint8_t expected_byte3 = expected_net_val & 0xFF;
    EXPECT_EQ(buffer[0], expected_byte0);
    EXPECT_EQ(buffer[1], expected_byte1);
    EXPECT_EQ(buffer[2], expected_byte2);
    EXPECT_EQ(buffer[3], expected_byte3);
}
