#pragma once
#include <vector>

struct packet_to_send{
    unsigned char destination_mac[6];
    std::vector<unsigned char> data;
};

template <typename T>
struct io<packet_to_send> {
    static std::vector<unsigned char> serialize(const packet_to_send& packet) {
        std::vector<unsigned char> buffer;
        buffer.insert(buffer.end(), packet.destination_mac, packet.destination_mac + sizeof(packet.destination_mac));
        buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
        return buffer;
    }

    static packet_to_send deserialize(std::span<const unsigned char> buffer) {
        packet_to_send packet;
        memcpy(packet.destination_mac, buffer.data(), sizeof(packet.destination_mac));
        buffer = buffer.subspan(sizeof(packet.destination_mac));
        packet.data.insert(packet.data.end(), buffer.begin(), buffer.end());
        return packet;
    }
};