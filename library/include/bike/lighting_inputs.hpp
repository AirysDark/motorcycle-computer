#pragma once

namespace bike {

class LightingInputs {
public:
    virtual ~LightingInputs() = default;

    virtual bool left_indicator_requested() const = 0;
    virtual bool right_indicator_requested() const = 0;
    virtual bool front_brake_active() const = 0;
    virtual bool rear_brake_active() const = 0;
    virtual bool high_beam_requested() const = 0;

    bool brake_active() const {
        return front_brake_active() || rear_brake_active();
    }
};

} // namespace bike
