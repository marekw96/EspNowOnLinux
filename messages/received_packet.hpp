#pragma once

#include "message_id.hpp"
#include <vector>
#include <span>
#include <cstdint>
#include "utility.hpp"

struct received_packet {

    message_id id = message_id::RECEIVED_PACKET;
    unsigned char mac[6];
    std::vector<unsigned char> data;
};

template <>
struct io<received_packet> {
    static std::vector<unsigned char> serialize(const received_packet& packet) {
        std::vector<unsigned char> buffer;
        buffer.insert(buffer.end(), static_cast<unsigned char>(packet.id));

        buffer.insert(buffer.end(), packet.mac, packet.mac + sizeof(packet.mac));

        uint32_t length = host_to_network(packet.data.size());
        buffer.insert(buffer.end(), reinterpret_cast<unsigned char*>(&length), reinterpret_cast<unsigned char*>(&length) + sizeof(length));
        buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
        return buffer;
    }

    static received_packet deserialize(std::span<const unsigned char> buffer) {
        received_packet packet;

        memcpy(packet.mac, buffer.data(), sizeof(packet.mac));
        buffer = buffer.subspan(sizeof(packet.mac));

        uint32_t length = network_to_host(*reinterpret_cast<const uint32_t*>(buffer.data()));
        buffer = buffer.subspan(sizeof(length));

        packet.data.insert(packet.data.end(), buffer.begin(), buffer.begin() + std::min(length, static_cast<uint32_t>(buffer.size())));
        return packet;
    }
};