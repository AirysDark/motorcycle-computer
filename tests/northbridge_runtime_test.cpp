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

    lighting.inject(make_packet(
        bike::NodeAddress::Lighting,
        bike::NodeAddress::Broadcast,
        bike::MessageType::Heartbeat));
    runtime.service(200);
    assert(pi.writes.size() == 1);
    assert(security.writes.size() == 1);

    // Source learning confirms the lighting node is associated with port 1.
    assert(runtime.router().route_for(bike::NodeAddress::Lighting) == 1);
    assert(runtime.router().is_online(bike::NodeAddress::Lighting, 250, 100));
    assert(!runtime.router().is_online(bike::NodeAddress::Lighting, 500, 100));

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
