#pragma once
#include <cstdint>
#include "bike/node.hpp"

namespace bike {

enum class LightingOutput : std::uint8_t {
    LeftIndicator  = 0x01,
    RightIndicator = 0x02,
    BrakeBright    = 0x03,
    HighBeam       = 0x04
};

class LightingClient {
public:
    explicit LightingClient(BikeNode& node) : node_(node) {}

    bool set_output(LightingOutput output, bool enabled, std::uint32_t now_ms, bool require_ack = true);
    bool set_left_indicator(bool enabled, std::uint32_t now_ms, bool require_ack = true) {
        return set_output(LightingOutput::LeftIndicator, enabled, now_ms, require_ack);
    }
    bool set_right_indicator(bool enabled, std::uint32_t now_ms, bool require_ack = true) {
        return set_output(LightingOutput::RightIndicator, enabled, now_ms, require_ack);
    }
    bool set_brake_bright(bool enabled, std::uint32_t now_ms, bool require_ack = true) {
        return set_output(LightingOutput::BrakeBright, enabled, now_ms, require_ack);
    }
    bool set_high_beam(bool enabled, std::uint32_t now_ms, bool require_ack = true) {
        return set_output(LightingOutput::HighBeam, enabled, now_ms, require_ack);
    }

private:
    BikeNode& node_;
};

} // namespace bike
