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

    void inject(const bike::Packet& packet) {
        std::array<std::uint8_t, bike::kMaxFrameSize> frame{};
        std::size_t length = 0;
        assert(bike::encode_packet(packet, frame.data(), frame.size(), length));
        for (std::size_t i = 0; i < length; ++i) rx.push_back(frame[i]);
    }

    bike::Packet decoded_write(std::size_t index) const {
        bike::Packet packet{};
        assert(index < writes.size());
        assert(bike::decode_packet(writes[index].data(), writes[index].size(), packet) == bike::DecodeStatus::Ok);
        return packet;
    }

    std::deque<std::uint8_t> rx;
    std::vector<std::vector<std::uint8_t>> writes;
};

static bike::Packet make_packet(
    bike::NodeAddress source,
    bike::NodeAddress destination,
    bike::MessageType type = bike::MessageType::Heartbeat) {
    bike::Packet packet{};
    packet.source = source;
    packet.destination = destination;
    packet.type = type;
    packet.sequence = 1;
    packet.length = 0;
    return packet;
}

int main() {
    MemoryTransport pi;
    MemoryTransport lighting;
    MemoryTransport security;

    bike::NorthbridgeRuntime runtime;
    assert(runtime.attach_port(0, pi));
    assert(runtime.attach_port(1, lighting));
    assert(runtime.attach_port(2, security));
    assert(runtime.set_route(bike::NodeAddress::MainComputer, 0));
    assert(runtime.set_route(bike::NodeAddress::Lighting, 1));
    assert(runtime.set_route(bike::NodeAddress::Security, 2));

    pi.inject(make_packet(
        bike::NodeAddress::MainComputer,
        bike::NodeAddress::Lighting,
        bike::MessageType::SetOutput));
    runtime.service(100);
    assert(lighting.writes.size() == 1);
    assert(security.writes.empty());
    assert(runtime.forwarded_packets() == 1);
    assert(runtime.port_stats(0).rx_packets == 1);
    assert(runtime.port_stats(1).tx_packets == 1);

    lighting.inject(make_packet(
        bike::NodeAddress::Lighting,
        bike::NodeAddress::Broadcast,
        bike::MessageType::Heartbeat));
    runtime.service(200);
    assert(pi.writes.size() == 1);
    assert(security.writes.size() == 1);

    assert(runtime.router().route_for(bike::NodeAddress::Lighting) == 1);
    assert(runtime.router().is_online(bike::NodeAddress::Lighting, 250, 100));
    assert(!runtime.router().is_online(bike::NodeAddress::Lighting, 500, 100));

    // Packets addressed to the northbridge are handled locally instead of swallowed.
    auto heartbeat = make_packet(
        bike::NodeAddress::MainComputer,
        bike::NodeAddress::Northbridge,
        bike::MessageType::Heartbeat);
    pi.inject(heartbeat);
    runtime.service(300);
    assert(pi.writes.size() == 2);
    auto heartbeat_reply = pi.decoded_write(1);
    assert(heartbeat_reply.source == bike::NodeAddress::Northbridge);
    assert(heartbeat_reply.destination == bike::NodeAddress::MainComputer);
    assert(heartbeat_reply.type == bike::MessageType::Heartbeat);

    auto diagnostic = make_packet(
        bike::NodeAddress::MainComputer,
        bike::NodeAddress::Northbridge,
        bike::MessageType::Diagnostic);
    pi.inject(diagnostic);
    runtime.service(400);
    assert(pi.writes.size() == 3);
    auto diagnostic_reply = pi.decoded_write(2);
    assert(diagnostic_reply.source == bike::NodeAddress::Northbridge);
    assert(diagnostic_reply.type == bike::MessageType::Diagnostic);
    assert(diagnostic_reply.payload[0] == 0x01);
    assert(diagnostic_reply.payload[1] == bike::kMaxRouterPorts);

    // Seeing a statically-known node on a different physical port is counted.
    security.inject(make_packet(
        bike::NodeAddress::Lighting,
        bike::NodeAddress::MainComputer,
        bike::MessageType::Heartbeat));
    runtime.service(500);
    assert(runtime.route_movement_events() == 1);
    assert(runtime.router().route_for(bike::NodeAddress::Lighting) == 2);

    // Unknown unicast has no route and is dropped rather than flooded.
    bike::Packet unknown = make_packet(
        bike::NodeAddress::MainComputer,
        static_cast<bike::NodeAddress>(0x44),
        bike::MessageType::Diagnostic);
    pi.inject(unknown);
    runtime.service(600);
    assert(runtime.dropped_packets() == 1);

    return 0;
}
