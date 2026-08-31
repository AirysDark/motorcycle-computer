#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "bike/node.hpp"
#include "bike/router.hpp"
#include "bike/stream_parser.hpp"

class MemoryTransport final : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        writes.emplace_back(data, data + length);
        return true;
    }

    std::size_t read(std::uint8_t* data, std::size_t capacity) override {
        std::size_t count = 0;
        while (count < capacity && !rx.empty()) {
            data[count++] = rx.front();
            rx.pop_front();
        }
        return count;
    }

    void inject(const std::vector<std::uint8_t>& frame) {
        for (auto byte : frame) rx.push_back(byte);
    }

    std::deque<std::uint8_t> rx;
    std::vector<std::vector<std::uint8_t>> writes;
};

int main() {
    using namespace bike;

    // Streaming parser must recover from leading garbage.
    Packet original{};
    original.source = NodeAddress::MainComputer;
    original.destination = NodeAddress::Lighting;
    original.type = MessageType::SetOutput;
    original.sequence = 42;
    original.length = 2;
    original.payload[0] = 0x01;
    original.payload[1] = 0x01;

    std::array<std::uint8_t, kMaxFrameSize> encoded{};
    std::size_t encoded_length = 0;
    assert(encode_packet(original, encoded.data(), encoded.size(), encoded_length));

    StreamParser parser;
    Packet parsed{};
    assert(!parser.push(0x00, parsed));
    assert(!parser.push(0x7E, parsed));
    bool got_packet = false;
    for (std::size_t i = 0; i < encoded_length; ++i) {
        got_packet = parser.push(encoded[i], parsed) || got_packet;
    }
    assert(got_packet);
    assert(parsed.sequence == 42);
    assert(parsed.destination == NodeAddress::Lighting);

    // Northbridge learns source ports and forwards to known destinations.
    MemoryTransport pi_port;
    MemoryTransport lighting_port;
    NorthbridgeRouter router;
    assert(router.attach_port(0, pi_port));
    assert(router.attach_port(1, lighting_port));
    assert(router.set_route(NodeAddress::MainComputer, 0));
    assert(router.set_route(NodeAddress::Lighting, 1));
    assert(router.forward(original, 0, 1000));
    assert(lighting_port.writes.size() == 1);
    assert(router.is_online(NodeAddress::MainComputer, 1100, 500));

    // End node sends ACK for an ACK-required command.
    MemoryTransport lighting_link;
    BikeNode lighting(NodeAddress::Lighting, lighting_link);
    Packet command = original;
    command.flags = FlagAckRequired;
    command.sequence = 77;

    std::array<std::uint8_t, kMaxFrameSize> command_frame{};
    std::size_t command_length = 0;
    assert(encode_packet(command, command_frame.data(), command_frame.size(), command_length));
    lighting_link.inject(std::vector<std::uint8_t>(command_frame.begin(), command_frame.begin() + command_length));

    Packet delivered{};
    assert(lighting.poll(delivered, 2000));
    assert(delivered.sequence == 77);
    assert(lighting_link.writes.size() == 1); // ACK frame

    return 0;
}
