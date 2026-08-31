#pragma once
#include <cstdint>
#include "bike/lighting_controller.hpp"
#include "bike/lighting_inputs.hpp"

namespace bike {

class LightingLocalControl {
public:
    LightingLocalControl(LightingController& controller, LightingHardware& outputs, LightingInputs& inputs)
        : controller_(controller), outputs_(outputs), inputs_(inputs) {}

    void set_blink_half_period_ms(std::uint32_t value) { blink_half_period_ms_ = value; }
    void service(std::uint32_t now_ms);

private:
    bool write_if_changed(LightingOutput output, bool enabled);
    bool local_indicator_active() const;

    LightingController& controller_;
    LightingHardware& outputs_;
    LightingInputs& inputs_;
    std::uint32_t blink_half_period_ms_{500};
    std::uint32_t last_blink_toggle_ms_{0};
    bool blink_on_{true};
    bool initialized_{false};
};

} // namespace bike
