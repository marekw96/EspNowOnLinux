#pragma once
#include <vector>
#include "message_id.hpp"

struct packet_to_send{
    message_id id = message_id::PACKET_TO_SEND;
    unsigned char destination_mac[6];
    std::vector<unsigned char> data;
};

template <>
struct io<packet_to_send> {
    static std::vector<unsigned char> serialize(const packet_to_send& packet) {
        std::vector<unsigned char> buffer;
        buffer.insert(buffer.end(), static_cast<unsigned char>(packet.id));
        buffer.insert(buffer.end(), packet.destination_mac, packet.destination_mac + sizeof(packet.destination_mac));
        uint32_t length = host_to_network(static_cast<uint32_t>(packet.data.size()));
        buffer.insert(buffer.end(), reinterpret_cast<unsigned char*>(&length), reinterpret_cast<unsigned char*>(&length) + sizeof(length));
        buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
        return buffer;
    }

    static packet_to_send deserialize(std::span<const unsigned char> buffer) {
        packet_to_send packet;
        buffer = buffer.subspan(1);
        memcpy(packet.destination_mac, buffer.data(), sizeof(packet.destination_mac));
        buffer = buffer.subspan(sizeof(packet.destination_mac));
        uint32_t length = network_to_host(*reinterpret_cast<const uint32_t*>(buffer.data()));
        buffer = buffer.subspan(sizeof(uint32_t));
        packet.data.assign(buffer.begin(), buffer.begin() + length);
        return packet;
    }
};