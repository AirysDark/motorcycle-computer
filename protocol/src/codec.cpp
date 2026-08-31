#include "bike_protocol/codec.hpp"
#include "bike_protocol/crc16.hpp"

namespace bike {
namespace {

void write_u16_be(std::uint8_t* out, std::uint16_t value) {
    out[0] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    out[1] = static_cast<std::uint8_t>(value & 0xFF);
}

std::uint16_t read_u16_be(const std::uint8_t* in) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(in[0]) << 8) |
        static_cast<std::uint16_t>(in[1]));
}

} // namespace

std::size_t encoded_size(const Packet& packet) {
    return kHeaderSize + packet.length + kCrcSize;
}

bool encode_packet(
    const Packet& packet,
    std::uint8_t* out,
    std::size_t out_capacity,
    std::size_t& out_length) {

    out_length = 0;

    if (packet.length > kMaxPayloadSize) {
        return false;
    }

    const std::size_t frame_size = encoded_size(packet);
    if (out == nullptr || out_capacity < frame_size) {
        return false;
    }

    out[0] = kSyncByte1;
    out[1] = kSyncByte2;
    out[2] = kProtocolVersion;
    out[3] = to_u8(packet.source);
    out[4] = to_u8(packet.destination);
    out[5] = static_cast<std::uint8_t>(packet.type);
    out[6] = packet.flags;
    write_u16_be(&out[7], packet.sequence);
    out[9] = static_cast<std::uint8_t>(packet.length);

    for (std::size_t i = 0; i < packet.length; ++i) {
        out[kHeaderSize + i] = packet.payload[i];
    }

    const std::uint16_t crc = crc16_ccitt_false(out, kHeaderSize + packet.length);
    write_u16_be(&out[kHeaderSize + packet.length], crc);

    out_length = frame_size;
    return true;
}

DecodeStatus decode_packet(
    const std::uint8_t* frame,
    std::size_t frame_length,
    Packet& out_packet) {

    if (frame == nullptr || frame_length < (kHeaderSize + kCrcSize)) {
        return DecodeStatus::FrameTooShort;
    }

    if (frame[0] != kSyncByte1 || frame[1] != kSyncByte2) {
        return DecodeStatus::InvalidSync;
    }

    if (frame[2] != kProtocolVersion) {
        return DecodeStatus::UnsupportedVersion;
    }

    const std::uint16_t payload_length = frame[9];
    if (payload_length > kMaxPayloadSize) {
        return DecodeStatus::PayloadTooLarge;
    }

    const std::size_t expected_size = kHeaderSize + payload_length + kCrcSize;
    if (frame_length != expected_size) {
        return DecodeStatus::LengthMismatch;
    }

    const std::uint16_t expected_crc = read_u16_be(&frame[kHeaderSize + payload_length]);
    const std::uint16_t actual_crc = crc16_ccitt_false(frame, kHeaderSize + payload_length);
    if (expected_crc != actual_crc) {
        return DecodeStatus::CrcMismatch;
    }

    out_packet.source = static_cast<NodeAddress>(frame[3]);
    out_packet.destination = static_cast<NodeAddress>(frame[4]);
    out_packet.type = static_cast<MessageType>(frame[5]);
    out_packet.flags = frame[6];
    out_packet.sequence = read_u16_be(&frame[7]);
    out_packet.length = payload_length;

    for (std::size_t i = 0; i < payload_length; ++i) {
        out_packet.payload[i] = frame[kHeaderSize + i];
    }

    return DecodeStatus::Ok;
}

} // namespace bike
