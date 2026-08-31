#pragma once
#include <cstddef>
#include <cstdint>
#include "packet.hpp"

namespace bike {

std::size_t encoded_size(const Packet& packet);

bool encode_packet(
    const Packet& packet,
    std::uint8_t* out,
    std::size_t out_capacity,
    std::size_t& out_length);

DecodeStatus decode_packet(
    const std::uint8_t* frame,
    std::size_t frame_length,
    Packet& out_packet);

} // namespace bike
