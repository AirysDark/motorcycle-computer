#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "bike_protocol/packet.hpp"

namespace bike {

class StreamParser {
public:
    bool push(std::uint8_t byte, Packet& out_packet);
    void reset();

private:
    std::array<std::uint8_t, kMaxFrameSize> buffer_{};
    std::size_t length_{0};
    std::size_t expected_{0};
};

} // namespace bike
