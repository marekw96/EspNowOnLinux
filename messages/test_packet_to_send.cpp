#include <gtest/gtest.h>
#include "packet_to_send.hpp"

TEST(PacketToSendTest, SerializationDeserialization) {
    packet_to_send packet;
    packet.id = message_id::PACKET_TO_SEND;
    packet.destination_mac[0] = 0x11;
    packet.destination_mac[1] = 0x22;
    packet.destination_mac[2] = 0x33;
    packet.destination_mac[3] = 0x44;
    packet.destination_mac[4] = 0x55;
    packet.destination_mac[5] = 0x66;
    packet.data = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E};

    // Serialize the packet
    auto serialized = io<packet_to_send>::serialize(packet);

    // Expected size: 1 byte ID + 6 bytes MAC + 5 bytes data = 12 bytes
    ASSERT_EQ(serialized.size(), 12);
    EXPECT_EQ(serialized[0], static_cast<unsigned char>(message_id::PACKET_TO_SEND));

    // Deserialize the packet
    // Note: io<packet_to_send>::deserialize explicitly drops the first byte (ID)
    auto deserialized = io<packet_to_send>::deserialize(std::span(serialized));

    EXPECT_EQ(deserialized.id, packet.id);
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(deserialized.destination_mac[i], packet.destination_mac[i]);
    }
    EXPECT_EQ(deserialized.data, packet.data);
}
