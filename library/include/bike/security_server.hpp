#pragma once
#include <cstdint>
#include "bike/security.hpp"
#include "bike/security_hardware.hpp"
#include "bike/security_persistence.hpp"

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
    SecurityServer(BikeNode& node, SecurityHardware& hardware, SecurityPersistence* persistence = nullptr)
        : node_(node), hardware_(hardware), persistence_(persistence) {}

    bool handle_packet(const Packet& packet, std::uint32_t now_ms);
    void service(std::uint32_t now_ms);
    bool publish_state(NodeAddress destination, std::uint32_t now_ms);
    bool restore_persisted_state(std::uint32_t now_ms);

    void set_engine_stop_confirm_ms(std::uint32_t value) { engine_stop_confirm_ms_ = value; }
    void set_shock_warning_confirm_ms(std::uint32_t value) { warning_confirm_ms_ = value; }
    void set_shock_trigger_confirm_ms(std::uint32_t value) { trigger_confirm_ms_ = value; }
    void set_alarm_duration_ms(std::uint32_t value) { alarm_duration_ms_ = value; }

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
    void sample_inputs(std::uint32_t now_ms);
    bool update_filtered_assert(bool raw, bool& stable, bool& pending,
                                std::uint32_t& changed_at_ms,
                                std::uint32_t confirm_ms,
                                std::uint32_t now_ms);
    void persist_armed(bool armed);

    BikeNode& node_;
    SecurityHardware& hardware_;
    SecurityPersistence* persistence_{nullptr};
    SecurityMode mode_{SecurityMode::Unlocked};
    NodeAddress last_controller_{NodeAddress::MainComputer};

    bool filtered_engine_running_{false};
    bool engine_filter_initialized_{false};
    bool engine_stop_pending_{false};
    std::uint32_t engine_stop_started_ms_{0};
    bool recovery_requires_stop_confirmation_{false};

    bool filtered_warning_{false};
    bool warning_pending_{false};
    std::uint32_t warning_changed_at_ms_{0};

    bool filtered_trigger_{false};
    bool trigger_pending_{false};
    std::uint32_t trigger_changed_at_ms_{0};

    bool previous_warning_{false};
    bool previous_trigger_{false};
    std::uint32_t alarm_started_ms_{0};
    bool alarm_timer_active_{false};

    std::uint32_t engine_stop_confirm_ms_{250};
    std::uint32_t warning_confirm_ms_{50};
    std::uint32_t trigger_confirm_ms_{100};
    std::uint32_t alarm_duration_ms_{30000};
};

} // namespace bike
