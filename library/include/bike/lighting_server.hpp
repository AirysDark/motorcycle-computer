#pragma once
#include <array>
#include <cstdint>
#include "bike/lighting_hardware.hpp"
#include "bike/node.hpp"

namespace bike {

struct LightingState {
    bool left_indicator{false};
    bool right_indicator{false};
    bool brake_bright{false};
    bool high_beam{false};
};

class LightingServer {
public:
    LightingServer(BikeNode& node, LightingHardware& hardware)
        : node_(node), hardware_(hardware) {}

    bool handle_packet(const Packet& packet, std::uint32_t now_ms);
    bool publish_state(NodeAddress destination, std::uint32_t now_ms);

    const LightingState& commanded_state() const { return commanded_; }
    LightingState actual_state() const;

private:
    bool valid_output(std::uint8_t raw) const;
    bool apply_output(LightingOutput output, bool enabled);
    void set_commanded(LightingOutput output, bool enabled);

    BikeNode& node_;
    LightingHardware& hardware_;
    LightingState commanded_{};
};

} // namespace bike
