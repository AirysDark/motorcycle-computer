#pragma once

namespace bike {

class SecurityHardware {
public:
    virtual ~SecurityHardware() = default;

    virtual bool set_alarm_output(bool enabled) = 0;
    virtual bool set_start_inhibit(bool enabled) = 0;

    virtual bool shock_warning_active() const = 0;
    virtual bool shock_trigger_active() const = 0;
    virtual bool engine_running() const = 0;

    virtual bool alarm_output_active() const = 0;
    virtual bool start_inhibit_active() const = 0;
};

} // namespace bike
