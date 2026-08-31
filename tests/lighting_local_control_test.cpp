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
    bool front_brake_active() const override { return front_brake; }
    bool rear_brake_active() const override { return rear_brake; }
    bool high_beam_requested() const override { return high; }

    bool left{false};
    bool right{false};
    bool front_brake{false};
    bool rear_brake{false};
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
    local.set_input_debounce_ms(25);

    // Initialize all inputs inactive.
    local.service(0);

    // A short brake bounce must not assert the lamp.
    inputs.front_brake = true;
    local.service(5);
    inputs.front_brake = false;
    local.service(15);
    assert(!outputs.read_output(bike::LightingOutput::BrakeBright));

    // Front brake asserts after the debounce interval.
    inputs.front_brake = true;
    local.service(30);
    local.service(54);
    assert(!outputs.read_output(bike::LightingOutput::BrakeBright));
    local.service(55);
    assert(outputs.read_output(bike::LightingOutput::BrakeBright));

    // Rear brake independently keeps the brake lamp active.
    inputs.rear_brake = true;
    local.service(60);
    local.service(85);
    inputs.front_brake = false;
    local.service(90);
    local.service(115);
    assert(outputs.read_output(bike::LightingOutput::BrakeBright));

    inputs.rear_brake = false;
    local.service(120);
    local.service(145);
    assert(!outputs.read_output(bike::LightingOutput::BrakeBright));

    // High beam is also debounced and locally authoritative.
    inputs.high = true;
    local.service(150);
    local.service(175);
    assert(outputs.read_output(bike::LightingOutput::HighBeam));
    inputs.high = false;
    local.service(180);
    local.service(205);
    assert(!outputs.read_output(bike::LightingOutput::HighBeam));

    // A local left indicator request flashes without any network command.
    inputs.left = true;
    local.service(210);
    local.service(235);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(734);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(735);
    assert(!outputs.read_output(bike::LightingOutput::LeftIndicator));
    local.service(1235);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));

    // Both local switches active behave as hazard flashing after debounce.
    inputs.right = true;
    local.service(1240);
    local.service(1265);
    local.service(1735);
    assert(!outputs.read_output(bike::LightingOutput::LeftIndicator));
    assert(!outputs.read_output(bike::LightingOutput::RightIndicator));
    local.service(2235);
    assert(outputs.read_output(bike::LightingOutput::LeftIndicator));
    assert(outputs.read_output(bike::LightingOutput::RightIndicator));

    return 0;
}
