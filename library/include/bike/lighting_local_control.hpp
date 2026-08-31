#pragma once
#include <cstdint>
#include "bike/lighting_controller.hpp"
#include "bike/lighting_inputs.hpp"

namespace bike {

class DebouncedInput {
public:
    bool update(bool raw, std::uint32_t now_ms, std::uint32_t debounce_ms);
    bool value() const { return stable_; }

private:
    bool raw_{false};
    bool stable_{false};
    bool initialized_{false};
    std::uint32_t changed_at_ms_{0};
};

class LightingLocalControl {
public:
    LightingLocalControl(LightingController& controller, LightingHardware& outputs, LightingInputs& inputs)
        : controller_(controller), outputs_(outputs), inputs_(inputs) {}

    void set_blink_half_period_ms(std::uint32_t value) { blink_half_period_ms_ = value; }
    void set_input_debounce_ms(std::uint32_t value) { input_debounce_ms_ = value; }
    void service(std::uint32_t now_ms);

private:
    bool write_if_changed(LightingOutput output, bool enabled);
    bool local_indicator_active() const;
    void sample_inputs(std::uint32_t now_ms);

    LightingController& controller_;
    LightingHardware& outputs_;
    LightingInputs& inputs_;
    DebouncedInput left_{};
    DebouncedInput right_{};
    DebouncedInput front_brake_{};
    DebouncedInput rear_brake_{};
    DebouncedInput high_beam_{};
    std::uint32_t input_debounce_ms_{25};
    std::uint32_t blink_half_period_ms_{500};
    std::uint32_t last_blink_toggle_ms_{0};
    bool blink_on_{true};
    bool indicator_active_previous_{false};
    bool initialized_{false};
};

} // namespace bike
