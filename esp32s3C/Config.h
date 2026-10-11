#pragma once
#include <Arduino.h>
// EDIT THESE PINS for your exact ESP32-S3 development board before wiring.
// All UART connections are 3.3 V TTL; NEVER connect motorcycle 12 V directly.
// GPIO numbers are placeholders, NOT a verified wiring diagram.
namespace config {
constexpr uint32_t BAUD=115200;
constexpr int MAIN_RX=4,MAIN_TX=5;
constexpr int LIGHT_RX=6,LIGHT_TX=7;
constexpr int SECURITY_RX=15,SECURITY_TX=16;
constexpr uint32_t STATS_INTERVAL_MS=10000;
}
