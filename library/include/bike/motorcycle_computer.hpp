#pragma once
#include <array>
#include <cstdint>
#include "bike/lighting.hpp"
#include "bike/lighting_diagnostics.hpp"
#include "bike/security.hpp"
#include "bike/supervisor.hpp"

namespace bike {

struct LightingSnapshot {
    bool valid{false};
    bool left_indicator{false};
    bool right_indicator{false};
    bool brake_bright{false};
    bool high_beam{false};
    std::uint32_t updated_at_ms{0};
};

struct LightingDiagnosticSnapshot {
    bool valid{false};
    std::array<LightingChannelDiagnostic, 4> channels{};
    std::uint32_t updated_at_ms{0};
};

struct SecuritySnapshot {
    bool valid{false};
    SecurityMode mode{SecurityMode::Unlocked};
    bool start_inhibit{false};
    bool alarm_active{false};
    bool shock_warning{false};
    bool shock_trigger{false};
    bool engine_running{false};
    SecurityEventCode last_event{SecurityEventCode::Unlocked};
    bool has_event{false};
    std::uint32_t updated_at_ms{0};
    std::uint32_t last_event_at_ms{0};
};

struct MotorcycleSnapshot {
    const NodeStatus* lighting_node{nullptr};
    const NodeStatus* security_node{nullptr};
    const NodeStatus* northbridge_node{nullptr};
    LightingSnapshot lighting{};
    LightingDiagnosticSnapshot lighting_diagnostics{};
    SecuritySnapshot security{};
    std::uint32_t tx_failures{0};
    std::uint32_t rx_drops{0};
};

class MotorcycleComputer {
public:
    MotorcycleComputer(BikeNode& node)
        : node_(node), lighting_(node), security_(node), supervisor_(node) {}

    LightingClient& lighting() { return lighting_; }
    SecurityClient& security() { return security_; }
    NetworkSupervisor& supervisor() { return supervisor_; }

    void begin(std::uint32_t now_ms);
    void service(std::uint32_t now_ms);
    MotorcycleSnapshot snapshot() const;

    bool request_all_states(std::uint32_t now_ms);

private:
    void dispatch(const Packet& packet, std::uint32_t now_ms);
    bool decode_lighting_state(const Packet& packet, std::uint32_t now_ms);
    bool decode_lighting_diagnostics(const Packet& packet, std::uint32_t now_ms);
    bool decode_security_state(const Packet& packet, std::uint32_t now_ms);
    bool decode_security_event(const Packet& packet, std::uint32_t now_ms);

    BikeNode& node_;
    LightingClient lighting_;
    SecurityClient security_;
    NetworkSupervisor supervisor_;
    LightingSnapshot lighting_state_{};
    LightingDiagnosticSnapshot lighting_diagnostics_{};
    SecuritySnapshot security_state_{};
};

} // namespace bike
