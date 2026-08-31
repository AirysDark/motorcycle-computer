#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "bike/northbridge.hpp"
#include "bike_protocol/codec.hpp"

class MemoryTransport final : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override { writes.emplace_back(data, data + length); return true; }
    std::size_t read(std::uint8_t* data, std::size_t capacity) override {
        std::size_t count = 0;
        while (count < capacity && !rx.empty()) { data[count++] = rx.front(); rx.pop_front(); }
        return count;
    }
    void inject(const bike::Packet& packet) {
        std::array<std::uint8_t, bike::kMaxFrameSize> frame{}; std::size_t length = 0;
        assert(bike::encode_packet(packet, frame.data(), frame.size(), length));
        for (std::size_t i = 0; i < length; ++i) rx.push_back(frame[i]);
    }
    bike::Packet decoded_write(std::size_t index) const {
        bike::Packet packet{}; assert(index < writes.size());
        assert(bike::decode_packet(writes[index].data(), writes[index].size(), packet) == bike::DecodeStatus::Ok);
        return packet;
    }
    std::deque<std::uint8_t> rx;
    std::vector<std::vector<std::uint8_t>> writes;
};

static bike::Packet make_packet(bike::NodeAddress source, bike::NodeAddress destination,
                                bike::MessageType type = bike::MessageType::Heartbeat) {
    bike::Packet packet{}; packet.source = source; packet.destination = destination;
    packet.type = type; packet.sequence = 1; return packet;
}

int main() {
    MemoryTransport pi, lighting, security;
    bike::NorthbridgeRuntime runtime;
    assert(runtime.attach_port(0, pi));
    assert(runtime.attach_port(1, lighting));
    assert(runtime.attach_port(2, security));
    assert(runtime.bind_node(bike::NodeAddress::MainComputer, 0));
    assert(runtime.bind_node(bike::NodeAddress::Lighting, 1));
    assert(runtime.bind_node(bike::NodeAddress::Security, 2));
    runtime.set_topology_enforced(true);

    pi.inject(make_packet(bike::NodeAddress::MainComputer, bike::NodeAddress::Lighting, bike::MessageType::SetOutput));
    runtime.service(100);
    assert(lighting.writes.size() == 1);
    assert(runtime.forwarded_packets() == 1);

    auto diagnostic = make_packet(bike::NodeAddress::MainComputer, bike::NodeAddress::Northbridge, bike::MessageType::Diagnostic);
    pi.inject(diagnostic);
    runtime.service(200);
    auto diagnostic_reply = pi.decoded_write(0);
    assert(diagnostic_reply.type == bike::MessageType::Diagnostic);
    assert(diagnostic_reply.payload[0] == 0x02);
    assert(diagnostic_reply.payload[1] == 3);

    // Lighting claiming its address on the security port is rejected and faulted.
    security.inject(make_packet(bike::NodeAddress::Lighting, bike::NodeAddress::MainComputer));
    runtime.service(300);
    assert(runtime.route_movement_events() == 1);
    assert(runtime.topology_fault_events() == 1);
    assert(runtime.router().route_for(bike::NodeAddress::Lighting) == 1);
    assert(runtime.dropped_packets() == 1);
    assert(pi.writes.size() == 2);
    const auto fault = pi.decoded_write(1);
    assert(fault.type == bike::MessageType::Fault);
    assert(fault.payload[0] == 0x02);
    assert(fault.payload[1] == 0x01);
    assert(fault.payload[2] == bike::to_u8(bike::NodeAddress::Lighting));
    assert(fault.payload[3] == 1);
    assert(fault.payload[4] == 2);

    // Corrupt a valid frame and confirm the ingress port records it as malformed.
    bike::Packet bad = make_packet(bike::NodeAddress::Security, bike::NodeAddress::MainComputer);
    std::array<std::uint8_t, bike::kMaxFrameSize> frame{}; std::size_t frame_length = 0;
    assert(bike::encode_packet(bad, frame.data(), frame.size(), frame_length));
    frame[frame_length - 1] ^= 0xFFu;
    for (std::size_t i = 0; i < frame_length; ++i) security.rx.push_back(frame[i]);
    runtime.service(400);
    assert(runtime.port_stats(2).malformed_frames == 1);

    return 0;
}
