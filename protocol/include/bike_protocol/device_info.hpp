#pragma once
#include <cstddef>
#include <cstdint>
#include "packet.hpp"

namespace bike {

enum class DeviceClass : std::uint8_t {
    MainComputer = 0x01,
    DiagnosticComputer = 0x02,
    NetworkController = 0x03,
    LightingController = 0x10,
    SecurityController = 0x11,
};

enum DeviceCapability : std::uint32_t {
    CapabilityNone = 0,
    CapabilityRouting = 1u << 0,
    CapabilityDiagnostics = 1u << 1,
    CapabilityLighting = 1u << 2,
    CapabilitySecurity = 1u << 3,
    CapabilityPersistentState = 1u << 4,
    CapabilityAlarm = 1u << 5,
    CapabilityStartInhibit = 1u << 6,
};

enum class ResetReason : std::uint8_t {
    Unknown = 0,
    PowerOn = 1,
    Software = 2,
    Watchdog = 3,
    Brownout = 4,
    External = 5,
    DeepSleep = 6,
};

struct DeviceIdentity {
    DeviceClass device_class{DeviceClass::MainComputer};
    std::uint8_t firmware_major{0};
    std::uint8_t firmware_minor{1};
    std::uint8_t firmware_patch{0};
    std::uint8_t hardware_revision{1};
    ResetReason reset_reason{ResetReason::Unknown};
    std::uint32_t capabilities{CapabilityNone};
};

constexpr std::uint8_t kDeviceInfoSchemaVersion = 1;
constexpr std::size_t kDeviceInfoPayloadSize = 16;

inline void put_device_u32(Packet& packet, std::size_t offset, std::uint32_t value) {
    packet.payload[offset] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    packet.payload[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    packet.payload[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    packet.payload[offset + 3] = static_cast<std::uint8_t>(value & 0xFFu);
}

inline void encode_device_info_payload(Packet& packet, NodeAddress address,
                                       const DeviceIdentity& identity,
                                       std::uint32_t uptime_ms) {
    packet.length = static_cast<std::uint16_t>(kDeviceInfoPayloadSize);
    packet.payload[0] = kDeviceInfoSchemaVersion;
    packet.payload[1] = to_u8(address);
    packet.payload[2] = static_cast<std::uint8_t>(identity.device_class);
    packet.payload[3] = kProtocolVersion;
    packet.payload[4] = identity.firmware_major;
    packet.payload[5] = identity.firmware_minor;
    packet.payload[6] = identity.firmware_patch;
    packet.payload[7] = identity.hardware_revision;
    packet.payload[8] = static_cast<std::uint8_t>(identity.reset_reason);
    packet.payload[9] = 0; // reserved
    put_device_u32(packet, 10, uptime_ms);
    packet.payload[14] = static_cast<std::uint8_t>((identity.capabilities >> 8) & 0xFFu);
    packet.payload[15] = static_cast<std::uint8_t>(identity.capabilities & 0xFFu);
}

} // namespace bike
