#include <array>
#include <cassert>
#include <cstdint>
#include "bike_protocol/codec.hpp"

int main() {
    bike::Packet tx;
    tx.source = bike::NodeAddress::MainComputer;
    tx.destination = bike::NodeAddress::Lighting;
    tx.type = bike::MessageType::SetOutput;
    tx.flags = bike::FlagAckRequired;
    tx.sequence = 0x1234;
    tx.length = 2;
    tx.payload[0] = 0x01; // left indicator
    tx.payload[1] = 0x01; // on

    std::array<std::uint8_t, bike::kMaxFrameSize> frame{};
    std::size_t frame_length = 0;

    assert(bike::encode_packet(tx, frame.data(), frame.size(), frame_length));

    bike::Packet rx;
    assert(bike::decode_packet(frame.data(), frame_length, rx) == bike::DecodeStatus::Ok);
    assert(rx.source == tx.source);
    assert(rx.destination == tx.destination);
    assert(rx.type == tx.type);
    assert(rx.flags == tx.flags);
    assert(rx.sequence == tx.sequence);
    assert(rx.length == tx.length);
    assert(rx.payload[0] == tx.payload[0]);
    assert(rx.payload[1] == tx.payload[1]);

    frame[10] ^= 0x01;
    assert(bike::decode_packet(frame.data(), frame_length, rx) == bike::DecodeStatus::CrcMismatch);

    return 0;
}
