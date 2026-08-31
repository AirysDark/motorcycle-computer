#include "bike/lighting_local_control.hpp"

namespace bike {

bool DebouncedInput::update(bool raw, std::uint32_t now_ms, std::uint32_t debounce_ms) {
    if (!initialized_) {
        raw_ = raw;
        stable_ = raw;
        changed_at_ms_ = now_ms;
        initialized_ = true;
        return false;
    }

    if (raw != raw_) {
        raw_ = raw;
        changed_at_ms_ = now_ms;
    }

    if (stable_ != raw_ && static_cast<std::uint32_t>(now_ms - changed_at_ms_) >= debounce_ms) {
        stable_ = raw_;
        return true;
    }

    return false;
}

void LightingLocalControl::sample_inputs(std::uint32_t now_ms) {
    left_.update(inputs_.left_indicator_requested(), now_ms, input_debounce_ms_);
    right_.update(inputs_.right_indicator_requested(), now_ms, input_debounce_ms_);
    front_brake_.update(inputs_.front_brake_active(), now_ms, input_debounce_ms_);
    rear_brake_.update(inputs_.rear_brake_active(), now_ms, input_debounce_ms_);
    high_beam_.update(inputs_.high_beam_requested(), now_ms, input_debounce_ms_);
}

bool LightingLocalControl::local_indicator_active() const {
    return left_.value() || right_.value();
}

bool LightingLocalControl::write_if_changed(LightingOutput output, bool enabled) {
    if (outputs_.read_output(output) == enabled) return false;
    outputs_.write_output(output, enabled);
    return true;
}

void LightingLocalControl::service(std::uint32_t now_ms) {
    sample_inputs(now_ms);
    const auto commanded = controller_.server().commanded_state();

    const bool local_left = left_.value();
    const bool local_right = right_.value();
    const bool indicator_active = local_indicator_active();

    if (!initialized_) {
        last_blink_toggle_ms_ = now_ms;
        blink_on_ = true;
        indicator_active_previous_ = indicator_active;
        initialized_ = true;
    }

    if (indicator_active && !indicator_active_previous_) {
        // Start a complete visible ON phase only after the input has passed
        // debounce. Raw switch-transition time must not shorten the first flash.
        blink_on_ = true;
        last_blink_toggle_ms_ = now_ms;
    } else if (indicator_active &&
               static_cast<std::uint32_t>(now_ms - last_blink_toggle_ms_) >= blink_half_period_ms_) {
        blink_on_ = !blink_on_;
        last_blink_toggle_ms_ = now_ms;
    }

    if (!indicator_active) {
        blink_on_ = true;
        last_blink_toggle_ms_ = now_ms;
    }

    indicator_active_previous_ = indicator_active;

    const bool effective_left = local_left ? blink_on_ : commanded.left_indicator;
    const bool effective_right = local_right ? blink_on_ : commanded.right_indicator;
    const bool effective_brake = front_brake_.value() || rear_brake_.value() || commanded.brake_bright;
    const bool effective_high = high_beam_.value() || commanded.high_beam;

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
