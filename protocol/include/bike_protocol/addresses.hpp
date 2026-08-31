#pragma once
#include <cstdint>

namespace bike {

enum class NodeAddress : std::uint8_t {
    MainComputer      = 0x01,
    DiagnosticComputer= 0x02,
    Northbridge       = 0x10,
    Lighting          = 0x20,
    Security          = 0x21,
    Broadcast         = 0xFF
};

constexpr std::uint8_t to_u8(NodeAddress address) {
    return static_cast<std::uint8_t>(address);
}

} // namespace bike
