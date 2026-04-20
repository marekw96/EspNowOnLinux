#include <gtest/gtest.h>
#include "received_packet.hpp"

TEST(ReceivedPacketTest, SerializationDeserialization) {
    received_packet packet;
    packet.id = message_id::RECEIVED_PACKET;
    packet.mac[0] = 0xAA;
    packet.mac[1] = 0xBB;
    packet.mac[2] = 0xCC;
    packet.mac[3] = 0xDD;
    packet.mac[4] = 0xEE;
    packet.mac[5] = 0xFF;
    packet.data = {0x01, 0x02, 0x03, 0x04};

    // Serialize the packet
    auto serialized = io<received_packet>::serialize(packet);

    // Expected size: 1 byte ID + 6 bytes MAC + 4 bytes length + 4 bytes data = 15 bytes
    ASSERT_EQ(serialized.size(), 15);
    EXPECT_EQ(serialized[0], static_cast<unsigned char>(message_id::RECEIVED_PACKET));

    // The generic io<T>::deserialize method drops the first byte (ID) and forwards to io<T>{}.deserialize
    auto deserialized = io<received_packet>::deserialize(std::span(serialized).subspan(1));

    EXPECT_EQ(deserialized.id, packet.id);
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(deserialized.mac[i], packet.mac[i]);
    }
    EXPECT_EQ(deserialized.data, packet.data);
}
