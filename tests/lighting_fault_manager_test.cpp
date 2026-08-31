#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bike/lighting_fault_manager.hpp"
#include "bike_protocol/codec.hpp"

namespace {
class NullTransport final : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override { writes.emplace_back(data, data + length); return true; }
    std::size_t read(std::uint8_t*, std::size_t) override { return 0; }
    std::vector<std::vector<std::uint8_t>> writes;
};
class FakeHardware final : public bike::LightingHardware {
public:
    bool write_output(bike::LightingOutput output, bool enabled) override { state[index(output)] = enabled; return true; }
    bool read_output(bike::LightingOutput output) const override { return state[index(output)]; }
    static std::size_t index(bike::LightingOutput output) { return static_cast<std::size_t>(static_cast<std::uint8_t>(output) - 1u); }
    bool state[4]{false, false, false, false};
};
class FakeFeedback final : public bike::LightingElectricalFeedback {
public:
    bool read_current_ma(bike::LightingOutput output, std::uint16_t& current_ma) const override {
        const auto i = FakeHardware::index(output); if (!available[i]) return false; current_ma = current[i]; return true;
    }
    bool available[4]{true, true, true, true};
    std::uint16_t current[4]{0, 0, 0, 0};
};
bike::Packet decode(const std::vector<std::uint8_t>& frame) {
    bike::Packet packet{}; assert(bike::decode_packet(frame.data(), frame.size(), packet) == bike::DecodeStatus::Ok); return packet;
}
}

int main() {
    NullTransport transport;
    FakeHardware hardware;
    FakeFeedback feedback;
    bike::BikeNode node(bike::NodeAddress::Lighting, transport);
    bike::LightingDiagnostics diagnostics(node, hardware, feedback);
    bike::LightingFaultManager faults(node, diagnostics, hardware);
    faults.set_confirm_samples(3);

    hardware.write_output(bike::LightingOutput::BrakeBright, true);
    feedback.current[2] = 10;
    faults.service(100); faults.service(200); assert(!faults.record(bike::LightingOutput::BrakeBright).latched);
    faults.service(300);
    const auto& brake = faults.record(bike::LightingOutput::BrakeBright);
    assert(brake.latched && brake.latched_status == bike::LightingElectricalStatus::OpenLoad);
    assert(brake.occurrence_count == 1 && brake.first_seen_ms == 300);
    assert(hardware.read_output(bike::LightingOutput::BrakeBright));
    assert(!transport.writes.empty());
    auto fault_packet = decode(transport.writes.back());
    assert(fault_packet.type == bike::MessageType::Fault && fault_packet.payload[1] == static_cast<std::uint8_t>(bike::LightingOutput::BrakeBright));

    assert(faults.clear_latch(bike::LightingOutput::BrakeBright, 350));
    assert(!faults.record(bike::LightingOutput::BrakeBright).latched);
    auto clear_packet = decode(transport.writes.back());
    assert(clear_packet.type == bike::MessageType::FaultClear);

    hardware.write_output(bike::LightingOutput::LeftIndicator, true);
    feedback.current[0] = 6000;
    faults.set_policy(bike::LightingOutput::LeftIndicator, bike::LightingFaultPolicy::ShutdownOnOverCurrent);
    faults.service(400); faults.service(500); faults.service(600);
    const auto& left = faults.record(bike::LightingOutput::LeftIndicator);
    assert(left.latched && left.latched_status == bike::LightingElectricalStatus::OverCurrent);
    assert(left.shutdown_applied && !hardware.read_output(bike::LightingOutput::LeftIndicator));
    return 0;
}
