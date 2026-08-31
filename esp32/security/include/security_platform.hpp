#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "bike/security_hardware.hpp"
#include "bike/security_persistence.hpp"
#include "bike/transport.hpp"

namespace security_platform {

// Initial bench defaults. Verify against the final PCB and motorcycle harness.
constexpr int kNetworkRxPin = 16;
constexpr int kNetworkTxPin = 17;
constexpr std::uint32_t kNetworkBaud = 115200;

constexpr int kShockWarningPin = 25;
constexpr int kShockTriggerPin = 26;
constexpr int kEngineRunningPin = 27;
constexpr int kAlarmOutputPin = 32;
constexpr int kStartInhibitPin = 33;

class ArduinoSerialTransport final : public bike::Transport {
public:
    explicit ArduinoSerialTransport(HardwareSerial& serial) : serial_(serial) {}

    bool write(const std::uint8_t* data, std::size_t length) override {
        return serial_.write(data, length) == length;
    }

    std::size_t read(std::uint8_t* data, std::size_t capacity) override {
        std::size_t count = 0;
        while (count < capacity && serial_.available() > 0) {
            const int value = serial_.read();
            if (value < 0) break;
            data[count++] = static_cast<std::uint8_t>(value);
        }
        return count;
    }

private:
    HardwareSerial& serial_;
};

class PreferencesSecurityPersistence final : public bike::SecurityPersistence {
public:
    bool begin() {
        opened_ = preferences_.begin("bike-security", false);
        return opened_;
    }

    bool load_armed(bool& armed) override {
        if (!opened_) return false;
        armed = preferences_.getBool("armed", false);
        return true;
    }

    bool save_armed(bool armed) override {
        if (!opened_) return false;
        return preferences_.putBool("armed", armed) == 1;
    }

private:
    Preferences preferences_{};
    bool opened_{false};
};

class GpioSecurityHardware final : public bike::SecurityHardware {
public:
    void begin() {
        digitalWrite(kAlarmOutputPin, LOW);
        digitalWrite(kStartInhibitPin, LOW);
        pinMode(kAlarmOutputPin, OUTPUT);
        pinMode(kStartInhibitPin, OUTPUT);
        digitalWrite(kAlarmOutputPin, LOW);
        digitalWrite(kStartInhibitPin, LOW);

        pinMode(kShockWarningPin, INPUT_PULLDOWN);
        pinMode(kShockTriggerPin, INPUT_PULLDOWN);
        pinMode(kEngineRunningPin, INPUT_PULLDOWN);
    }

    bool set_alarm_output(bool enabled) override {
        digitalWrite(kAlarmOutputPin, enabled ? HIGH : LOW);
        alarm_ = enabled;
        return true;
    }

    bool set_start_inhibit(bool enabled) override {
        // Logical HIGH currently means "prevent start" for the bench adapter.
        // The final relay/driver stage must preserve the rule that this output
        // cannot remove ignition from an already-running motorcycle.
        digitalWrite(kStartInhibitPin, enabled ? HIGH : LOW);
        inhibit_ = enabled;
        return true;
    }

    bool shock_warning_active() const override {
        return digitalRead(kShockWarningPin) == HIGH;
    }

    bool shock_trigger_active() const override {
        return digitalRead(kShockTriggerPin) == HIGH;
    }

    bool engine_running() const override {
        return digitalRead(kEngineRunningPin) == HIGH;
    }

    bool alarm_output_active() const override { return alarm_; }
    bool start_inhibit_active() const override { return inhibit_; }

private:
    bool alarm_{false};
    bool inhibit_{false};
};

} // namespace security_platform
