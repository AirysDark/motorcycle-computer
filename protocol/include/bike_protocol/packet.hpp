#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "addresses.hpp"
#include "messages.hpp"

namespace bike {

constexpr std::uint8_t kSyncByte1 = 0xA5;
constexpr std::uint8_t kSyncByte2 = 0x5A;
constexpr std::uint8_t kProtocolVersion = 0x01;
constexpr std::size_t kMaxPayloadSize = 128;
constexpr std::size_t kHeaderSize = 10;
constexpr std::size_t kCrcSize = 2;
constexpr std::size_t kMaxFrameSize = kHeaderSize + kMaxPayloadSize + kCrcSize;

struct Packet {
    NodeAddress source{NodeAddress::MainComputer};
    NodeAddress destination{NodeAddress::Northbridge};
    MessageType type{MessageType::Heartbeat};
    std::uint8_t flags{FlagNone};
    std::uint16_t sequence{0};
    std::uint16_t length{0};
    std::array<std::uint8_t, kMaxPayloadSize> payload{};
};

enum class DecodeStatus {
    Ok,
    FrameTooShort,
    InvalidSync,
    UnsupportedVersion,
    PayloadTooLarge,
    LengthMismatch,
    CrcMismatch
};

} // namespace bike
