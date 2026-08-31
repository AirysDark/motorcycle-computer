#pragma once
#include <array>
#include <cstdint>
#include "bike/lighting_hardware.hpp"
#include "bike/node.hpp"

namespace bike {

enum class LightingElectricalStatus : std::uint8_t {
    Unknown     = 0x00,
    Off         = 0x01,
    Ok          = 0x02,
    OpenLoad    = 0x03,
    OverCurrent = 0x04
};

struct LightingCurrentLimits {
    std::uint16_t open_load_below_ma{50};
    std::uint16_t over_current_above_ma{5000};
};

struct LightingChannelDiagnostic {
    LightingOutput output{LightingOutput::LeftIndicator};
    bool driven{false};
    bool feedback_available{false};
    LightingElectricalStatus status{LightingElectricalStatus::Unknown};
    std::uint16_t current_ma{0};
};

class LightingElectricalFeedback {
public:
    virtual ~LightingElectricalFeedback() = default;
    virtual bool read_current_ma(LightingOutput output, std::uint16_t& current_ma) const = 0;
};

class LightingDiagnostics {
public:
    LightingDiagnostics(BikeNode& node, const LightingHardware& outputs, const LightingElectricalFeedback& feedback)
        : node_(node), outputs_(outputs), feedback_(feedback) {}

    void set_limits(LightingOutput output, LightingCurrentLimits limits);
    void set_publish_interval_ms(std::uint32_t value) { publish_interval_ms_ = value; }

    LightingChannelDiagnostic evaluate(LightingOutput output) const;
    bool publish(NodeAddress destination, std::uint32_t now_ms);
    void service(std::uint32_t now_ms, NodeAddress destination = NodeAddress::MainComputer);

private:
    static std::size_t index_for(LightingOutput output);

    BikeNode& node_;
    const LightingHardware& outputs_;
    const LightingElectricalFeedback& feedback_;
    std::array<LightingCurrentLimits, 4> limits_{};
    std::uint32_t publish_interval_ms_{1000};
    std::uint32_t last_publish_ms_{0};
    bool published_once_{false};
};

} // namespace bike
