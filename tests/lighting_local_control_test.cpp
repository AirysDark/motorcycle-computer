#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bike/lighting_local_control.hpp"

namespace {

class NullTransport final : public bike::Transport {
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

    bool read_output(bike::LightingOutput output) const override {
        return state[index(output)];
    }

    static std::size_t index(bike::LightingOutput output) {
        return static_cast<std::size_t>(static_cast<std::uint8_t>(output) - 1u);
    }

    bool state[4]{false, false, false, false};
};

class FakeInputs final : public bike::LightingInputs {
public:
    bool left_indicator_requested() const override { return left; }
    bool right_indicator_requested() const override { return right; }
    bool brake_active() const override { return brake; }
    bool high_beam_requested() const override { return high; }

    bool left{false};
    bool right{false};
    bool brake{false};
    bool high{false};
};

} // namespace

int main() {
    NullTransport transport;
    FakeOutputs outputs;
    FakeInputs inputs;
    bike::BikeNode node(bike::NodeAddress::Lighting, transport);
    bike::LightingController controller(node, outputs);
    bike::LightingLocalControl local(controller, outputs, inputs);
    local.set_blink_half_period_ms(500);

    // Physical brake and high-beam requests immediately force outputs on.
    inputs.brake = true;
    inputs.high = true;
    local.service(0);
    assert(outputs.read_output(bike::LightingOutput::BrakeBright));
    assert(outputs.read_output(bike::LightingOutput::HighBeam));

    // Removing local requests returns to the remote commanded state (default OFF).
    inputs.brake = false;
    inputs.high = false;
    local.service(1);
    assert(!outputs.read_output(bike::LightingOutput::BrakeBright));
    assert(!outputs.read_output(bike::LightingOutput::HighBeam));

    // A local left indicator request flashes without any network command.
    inputs.left = true;
    local.service(100);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(599);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(600);
    assert(!outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(1100);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));

    // Both local switches active behave as hazard flashing.
    inputs.right = true;
    local.service(1600);
    assert(!outputs.read_output(bike::LightingOutput::LeftIndicator));
    assert(!outputs.read_output(bike::LightingOutput::RightIndicator));
    local.service(2100);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    assert(outputs.read_output(bike::LightingOutput::RightIndicator));

    return 0;
}
