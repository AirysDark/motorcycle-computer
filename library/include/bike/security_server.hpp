#pragma once
#include <cstdint>
#include "bike/security.hpp"
#include "bike/security_hardware.hpp"

namespace bike {

struct SecurityStateSnapshot {
    SecurityMode mode{SecurityMode::Unlocked};
    bool start_inhibit{false};
    bool alarm_active{false};
    bool shock_warning{false};
    bool shock_trigger{false};
    bool engine_running{false};
};

class SecurityServer {
public:
    SecurityServer(BikeNode& node, SecurityHardware& hardware)
        : node_(node), hardware_(hardware) {}

    bool handle_packet(const Packet& packet, std::uint32_t now_ms);
    void service(std::uint32_t now_ms);
    bool publish_state(NodeAddress destination, std::uint32_t now_ms);

    SecurityStateSnapshot state() const;
    SecurityMode mode() const { return mode_; }

private:
    bool handle_command(SecurityCommand command, NodeAddress source, std::uint16_t sequence,
                        bool ack_required, std::uint32_t now_ms);
    bool set_locked(bool locked, std::uint32_t now_ms);
    bool start_alarm(NodeAddress report_to, std::uint32_t now_ms);
    bool stop_alarm(NodeAddress report_to, std::uint32_t now_ms);
    bool publish_event(SecurityEventCode event, NodeAddress destination, std::uint32_t now_ms);
    void update_start_inhibit();

    BikeNode& node_;
    SecurityHardware& hardware_;
    SecurityMode mode_{SecurityMode::Unlocked};
    NodeAddress last_controller_{NodeAddress::MainComputer};
    bool previous_warning_{false};
    bool previous_trigger_{false};
};

} // namespace bike
