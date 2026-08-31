#pragma once

#include <Arduino.h>
#include "bike/transport.hpp"

namespace northbridge_platform {

// Bench defaults for ESP32-S3 DevKitC. Verify against final board/wiring.
constexpr std::uint32_t kNetworkBaud = 115200;

constexpr std::uint8_t kPortPi = 0;
constexpr std::uint8_t kPortLighting = 1;
constexpr std::uint8_t kPortSecurity = 2;

constexpr int kPiRxPin = 4;
constexpr int kPiTxPin = 5;
constexpr int kLightingRxPin = 16;
constexpr int kLightingTxPin = 17;
constexpr int kSecurityRxPin = 18;
constexpr int kSecurityTxPin = 21;

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

} // namespace northbridge_platform
