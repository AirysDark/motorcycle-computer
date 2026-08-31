#pragma once

namespace bike {

class LightingInputs {
public:
    virtual ~LightingInputs() = default;

    virtual bool left_indicator_requested() const = 0;
    virtual bool right_indicator_requested() const = 0;
    virtual bool brake_active() const = 0;
    virtual bool high_beam_requested() const = 0;
};

} // namespace bike
