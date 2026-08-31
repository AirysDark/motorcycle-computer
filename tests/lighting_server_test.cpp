#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "bike/lighting_server.hpp"
#include "bike_protocol/codec.hpp"

namespace {

class BufferTransport : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        writes.emplace_back(data, data + length);
        return true;
    }

    std::size_t read(std::uint8_t*, std::size_t) override { return 0; }

    std::vector<std::vector<std::uint8_t>> writes;
};

class FakeLightingHardware : public bike::LightingHardware {
public:
    bool write_output(bike::LightingOutput output, bool enabled) override {
        if (fail_writes) return false;
        set(output, enabled);
        return true;
    }

    bool read_output(bike::LightingOutput output) const override {
        return get(output);
    }

    void set(bike::LightingOutput output, bool enabled) {
        switch (output) {
            case bike::LightingOutput::LeftIndicator: left = enabled; break;
            case bike::LightingOutput::RightIndicator: right = enabled; break;
            case bike::LightingOutput::BrakeBright: brake = enabled; break;
            case bike::LightingOutput::HighBeam: high = enabled; break;
        }
    }

    bool get(bike::LightingOutput output) const {
        switch (output) {
            case bike::LightingOutput::LeftIndicator: return left;
            case bike::LightingOutput::RightIndicator: return right;
            case bike::LightingOutput::BrakeBright: return brake;
            case bike::LightingOutput::HighBeam: return high;
        }
        return false;
    }

    bool left{false};
    bool right{false};
    bool brake{false};
    bool high{false};
    bool fail_writes{false};
};

bike::Packet decode(const std::vector<std::uint8_t>& frame) {
    bike::Packet packet{};
    const auto status = bike::decode_packet(frame.data(), frame.size(), packet);
    assert(status == bike::DecodeStatus::Ok);
    return packet;
}

} // namespace

int main() {
    BufferTransport transport;
    bike::BikeNode node(bike::NodeAddress::Lighting, transport);
    node.set_auto_ack(false);
    FakeLightingHardware hardware;
    bike::LightingServer server(node, hardware);

    bike::Packet command{};
    command.source = bike::NodeAddress::MainComputer;
    command.destination = bike::NodeAddress::Lighting;
    command.type = bike::MessageType::SetOutput;
    command.flags = bike::FlagAckRequired;
    command.sequence = 42;
    command.length = 2;
    command.payload[0] = static_cast<std::uint8_t>(bike::LightingOutput::LeftIndicator);
    command.payload[1] = 1;

    assert(server.handle_packet(command, 100));
    assert(hardware.left);
    assert(server.commanded_state().left_indicator);
    assert(transport.writes.size() == 2);

    auto ack = decode(transport.writes[0]);
    assert(ack.type == bike::MessageType::Ack);
    assert(ack.sequence == 42);
    assert(ack.destination == bike::NodeAddress::MainComputer);

    auto state = decode(transport.writes[1]);
    assert(state.type == bike::MessageType::StateUpdate);
    assert(state.length == 8);
    assert(state.payload[0] == static_cast<std::uint8_t>(bike::LightingOutput::LeftIndicator));
    assert(state.payload[1] == 1);

    transport.writes.clear();
    command.sequence = 43;
    command.payload[0] = 0xFE;
    assert(server.handle_packet(command, 200));
    assert(transport.writes.size() == 1);
    auto nack = decode(transport.writes[0]);
    assert(nack.type == bike::MessageType::Nack);
    assert(nack.sequence == 43);

    transport.writes.clear();
    hardware.fail_writes = true;
    command.sequence = 44;
    command.payload[0] = static_cast<std::uint8_t>(bike::LightingOutput::HighBeam);
    command.payload[1] = 1;
    assert(server.handle_packet(command, 300));
    assert(transport.writes.size() == 1);
    nack = decode(transport.writes[0]);
    assert(nack.type == bike::MessageType::Nack);

    return 0;
}
