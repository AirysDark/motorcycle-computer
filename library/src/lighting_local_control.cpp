#include "bike/lighting_local_control.hpp"

namespace bike {

bool LightingLocalControl::local_indicator_active() const {
    return inputs_.left_indicator_requested() || inputs_.right_indicator_requested();
}

bool LightingLocalControl::write_if_changed(LightingOutput output, bool enabled) {
    if (outputs_.read_output(output) == enabled) return false;
    outputs_.write_output(output, enabled);
    return true;
}

void LightingLocalControl::service(std::uint32_t now_ms) {
    const auto commanded = controller_.server().commanded_state();

    const bool local_left = inputs_.left_indicator_requested();
    const bool local_right = inputs_.right_indicator_requested();

    if (!initialized_) {
        last_blink_toggle_ms_ = now_ms;
        blink_on_ = true;
        initialized_ = true;
    }

    if (local_indicator_active() &&
        static_cast<std::uint32_t>(now_ms - last_blink_toggle_ms_) >= blink_half_period_ms_) {
        blink_on_ = !blink_on_;
        last_blink_toggle_ms_ = now_ms;
    }

    if (!local_indicator_active()) {
        blink_on_ = true;
        last_blink_toggle_ms_ = now_ms;
    }

    const bool effective_left = local_left ? blink_on_ : commanded.left_indicator;
    const bool effective_right = local_right ? blink_on_ : commanded.right_indicator;
    const bool effective_brake = inputs_.brake_active() || commanded.brake_bright;
    const bool effective_high = inputs_.high_beam_requested() || commanded.high_beam;

    bool changed = false;
    changed = write_if_changed(LightingOutput::LeftIndicator, effective_left) || changed;
    changed = write_if_changed(LightingOutput::RightIndicator, effective_right) || changed;
    changed = write_if_changed(LightingOutput::BrakeBright, effective_brake) || changed;
    changed = write_if_changed(LightingOutput::HighBeam, effective_high) || changed;

    if (changed) {
        controller_.server().publish_state(NodeAddress::MainComputer, now_ms);
    }
}

} // namespace bike
