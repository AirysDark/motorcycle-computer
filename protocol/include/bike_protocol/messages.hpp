#pragma once
#include <cstdint>

namespace bike {

enum class MessageType : std::uint8_t {
    Heartbeat       = 0x01,
    Ack             = 0x02,
    Nack            = 0x03,

    GetState        = 0x10,
    StateUpdate     = 0x11,

    SetOutput       = 0x20,
    InputEvent      = 0x21,

    ConfigRead      = 0x30,
    ConfigWrite     = 0x31,

    Fault           = 0x40,
    FaultClear      = 0x41,

    DeviceInfo      = 0x50,
    DeviceDiscovery = 0x51,

    Diagnostic      = 0x60
};

enum PacketFlags : std::uint8_t {
    FlagNone        = 0x00,
    FlagAckRequired = 0x01,
    FlagResponse    = 0x02,
    FlagFault       = 0x04
};

} // namespace bike
