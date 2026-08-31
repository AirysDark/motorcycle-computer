#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include "bike/lighting_hardware.hpp"
#include "bike/lighting_inputs.hpp"
#include "bike/transport.hpp"

namespace lighting_platform {

// Initial bench defaults for a generic ESP32-WROOM dev board.
// These are deliberately centralized and must be verified against the final PCB/wiring.
constexpr int kNetworkRxPin = 16;
constexpr int kNetworkTxPin = 17;
constexpr std::uint32_t kNetworkBaud = 115200;

constexpr int kLeftIndicatorPin  = 25;
constexpr int kRightIndicatorPin = 26;
constexpr int kBrakeBrightPin    = 27;
constexpr int kHighBeamPin       = 32;

enum class InputPolarity : std::uint8_t {
    ActiveLow,
    ActiveHigh
};

// Bench input defaults. Each input has an explicit polarity so the final
// conditioned motorcycle interface can change electrical sense without
// changing the portable lighting-control logic.
constexpr int kLeftSwitchPin       = 18;
constexpr int kRightSwitchPin      = 19;
constexpr int kFrontBrakeInputPin  = 21;
constexpr int kRearBrakeInputPin   = 23;
constexpr int kHighBeamInputPin    = 22;

constexpr InputPolarity kLeftSwitchPolarity      = InputPolarity::ActiveLow;
constexpr InputPolarity kRightSwitchPolarity     = InputPolarity::ActiveLow;
constexpr InputPolarity kFrontBrakePolarity      = InputPolarity::ActiveLow;
constexpr InputPolarity kRearBrakePolarity       = InputPolarity::ActiveLow;
constexpr InputPolarity kHighBeamSwitchPolarity  = InputPolarity::ActiveLow;

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

class GpioLightingHardware final : public bike::LightingHardware {
public:
    void begin() {
        configure_output(kLeftIndicatorPin);
        configure_output(kRightIndicatorPin);
        configure_output(kBrakeBrightPin);
        configure_output(kHighBeamPin);
    }

    bool write_output(bike::LightingOutput output, bool enabled) override {
        const int pin = pin_for(output);
        const auto index = index_for(output);
        if (pin < 0 || index >= 4) return false;

        digitalWrite(pin, enabled ? HIGH : LOW);
        state_[index] = enabled;
        return true;
    }

    bool read_output(bike::LightingOutput output) const override {
        const auto index = index_for(output);
        return index < 4 ? state_[index] : false;
    }

private:
    static void configure_output(int pin) {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    static int pin_for(bike::LightingOutput output) {
        switch (output) {
            case bike::LightingOutput::LeftIndicator:  return kLeftIndicatorPin;
            case bike::LightingOutput::RightIndicator: return kRightIndicatorPin;
            case bike::LightingOutput::BrakeBright:    return kBrakeBrightPin;
            case bike::LightingOutput::HighBeam:       return kHighBeamPin;
        }
        return -1;
    }

    static std::size_t index_for(bike::LightingOutput output) {
        switch (output) {
            case bike::LightingOutput::LeftIndicator:  return 0;
            case bike::LightingOutput::RightIndicator: return 1;
            case bike::LightingOutput::BrakeBright:    return 2;
            case bike::LightingOutput::HighBeam:       return 3;
        }
        return 4;
    }

    bool state_[4]{false, false, false, false};
};

class GpioLightingInputs final : public bike::LightingInputs {
public:
    void begin() {
        configure_input(kLeftSwitchPin, kLeftSwitchPolarity);
        configure_input(kRightSwitchPin, kRightSwitchPolarity);
        configure_input(kFrontBrakeInputPin, kFrontBrakePolarity);
        configure_input(kRearBrakeInputPin, kRearBrakePolarity);
        configure_input(kHighBeamInputPin, kHighBeamSwitchPolarity);
    }

    bool left_indicator_requested() const override {
        return active(kLeftSwitchPin, kLeftSwitchPolarity);
    }

    bool right_indicator_requested() const override {
        return active(kRightSwitchPin, kRightSwitchPolarity);
    }

    bool front_brake_active() const override {
        return active(kFrontBrakeInputPin, kFrontBrakePolarity);
    }

    bool rear_brake_active() const override {
        return active(kRearBrakeInputPin, kRearBrakePolarity);
    }

    bool high_beam_requested() const override {
        return active(kHighBeamInputPin, kHighBeamSwitchPolarity);
    }

private:
    static void configure_input(int pin, InputPolarity polarity) {
        pinMode(pin, polarity == InputPolarity::ActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);
    }

    static bool active(int pin, InputPolarity polarity) {
        const int level = digitalRead(pin);
        return polarity == InputPolarity::ActiveLow ? level == LOW : level == HIGH;
    }
};

} // namespace lighting_platform
