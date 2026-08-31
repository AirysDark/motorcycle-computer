#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bike/lighting_diagnostics.hpp"

namespace {

class BufferTransport final : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        writes.emplace_back(data, data + length);
        return true;
    }
    std::size_t read(std::uint8_t*, std::size_t) override { return 0; }
    std::vector<std::vector<std::uint8_t>> writes;
};

class FakeOutputs final : public bike::LightingHardware {
public:
    bool write_output(bike::LightingOutput output, bool enabled) override {
        state[index(output)] = enabled;
        return true;
    }
    bool read_output(bike::LightingOutput output) const override { return state[index(output)]; }
    static std::size_t index(bike::LightingOutput output) {
        return static_cast<std::size_t>(static_cast<std::uint8_t>(output) - 1u);
    }
    bool state[4]{false, false, false, false};
};

class FakeFeedback final : public bike::LightingElectricalFeedback {
public:
    bool read_current_ma(bike::LightingOutput output, std::uint16_t& current_ma) const override {
        const auto i = static_cast<std::size_t>(static_cast<std::uint8_t>(output) - 1u);
        current_ma = current[i];
        return available[i];
    }
    bool available[4]{false, false, false, false};
    std::uint16_t current[4]{0, 0, 0, 0};
};

} // namespace

int main() {
    BufferTransport transport;
    FakeOutputs outputs;
    FakeFeedback feedback;
    bike::BikeNode node(bike::NodeAddress::Lighting, transport);
    bike::LightingDiagnostics diagnostics(node, outputs, feedback);

    auto d = diagnostics.evaluate(bike::LightingOutput::LeftIndicator);
    assert(d.status == bike::LightingElectricalStatus::Off);

    outputs.write_output(bike::LightingOutput::LeftIndicator, true);
    d = diagnostics.evaluate(bike::LightingOutput::LeftIndicator);
    assert(d.status == bike::LightingElectricalStatus::Unknown);
    assert(!d.feedback_available);

    feedback.available[0] = true;
    feedback.current[0] = 1200;
    d = diagnostics.evaluate(bike::LightingOutput::LeftIndicator);
    assert(d.status == bike::LightingElectricalStatus::Ok);
    assert(d.current_ma == 1200);

    feedback.current[0] = 10;
    d = diagnostics.evaluate(bike::LightingOutput::LeftIndicator);
    assert(d.status == bike::LightingElectricalStatus::OpenLoad);

    feedback.current[0] = 6000;
    d = diagnostics.evaluate(bike::LightingOutput::LeftIndicator);
    assert(d.status == bike::LightingElectricalStatus::OverCurrent);

    assert(diagnostics.publish(bike::NodeAddress::MainComputer, 100));
    assert(!transport.writes.empty());

    return 0;
}
