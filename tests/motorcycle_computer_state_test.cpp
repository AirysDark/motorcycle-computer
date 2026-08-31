#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "bike/motorcycle_computer.hpp"
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

int main() {
    using namespace bike;

    MemoryTransport transport;
    BikeNode node(NodeAddress::MainComputer, transport);
    MotorcycleComputer computer(node);

    Packet lighting{};
    lighting.source = NodeAddress::Lighting;
    lighting.destination = NodeAddress::MainComputer;
    lighting.type = MessageType::StateUpdate;
    lighting.sequence = 10;
    lighting.length = 8;
    lighting.payload[0] = static_cast<std::uint8_t>(LightingOutput::LeftIndicator);
    lighting.payload[1] = 1;
    lighting.payload[2] = static_cast<std::uint8_t>(LightingOutput::RightIndicator);
    lighting.payload[3] = 0;
    lighting.payload[4] = static_cast<std::uint8_t>(LightingOutput::BrakeBright);
    lighting.payload[5] = 1;
    lighting.payload[6] = static_cast<std::uint8_t>(LightingOutput::HighBeam);
    lighting.payload[7] = 0;

    Packet security{};
    security.source = NodeAddress::Security;
    security.destination = NodeAddress::MainComputer;
    security.type = MessageType::SecurityState;
    security.sequence = 11;
    security.length = 6;
    security.payload[0] = static_cast<std::uint8_t>(SecurityMode::Locked);
    security.payload[1] = 1;
    security.payload[2] = 0;
    security.payload[3] = 1;
    security.payload[4] = 0;
    security.payload[5] = 0;

    // Inject both frames before one service pass. BikeNode must preserve the
    // second complete frame instead of dropping the unread bytes after the first.
    transport.inject(lighting);
    transport.inject(security);

    computer.service(5000);

    const auto snapshot = computer.snapshot();
    assert(snapshot.lighting.valid);
    assert(snapshot.lighting.left_indicator);
    assert(!snapshot.lighting.right_indicator);
    assert(snapshot.lighting.brake_bright);
    assert(!snapshot.lighting.high_beam);

    assert(snapshot.security.valid);
    assert(snapshot.security.mode == SecurityMode::Locked);
    assert(snapshot.security.start_inhibit);
    assert(!snapshot.security.alarm_active);
    assert(snapshot.security.shock_warning);
    assert(!snapshot.security.shock_trigger);
    assert(!snapshot.security.engine_running);

    assert(snapshot.rx_drops == 0);
    assert(snapshot.lighting_node != nullptr && snapshot.lighting_node->online);
    assert(snapshot.security_node != nullptr && snapshot.security_node->online);

    return 0;
}
