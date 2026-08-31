#include "bike/security_server.hpp"

namespace bike {

SecurityStateSnapshot SecurityServer::state() const {
    SecurityStateSnapshot snapshot{};
    snapshot.mode = mode_;
    snapshot.start_inhibit = hardware_.start_inhibit_active();
    snapshot.alarm_active = hardware_.alarm_output_active();
    snapshot.shock_warning = hardware_.shock_warning_active();
    snapshot.shock_trigger = hardware_.shock_trigger_active();
    snapshot.engine_running = hardware_.engine_running();
    return snapshot;
}

void SecurityServer::update_start_inhibit() {
    // Critical rule: never assert the inhibit while the engine is already running.
    // Locking a running motorcycle is therefore deferred until the engine stops.
    const bool should_inhibit = (mode_ != SecurityMode::Unlocked) && !hardware_.engine_running();
    hardware_.set_start_inhibit(should_inhibit);
}

bool SecurityServer::publish_event(SecurityEventCode event, NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::SecurityEvent;
    packet.length = 1;
    packet.payload[0] = static_cast<std::uint8_t>(event);
    return node_.send(packet, now_ms, false);
}

bool SecurityServer::publish_state(NodeAddress destination, std::uint32_t now_ms) {
    const auto snapshot = state();
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::SecurityState;
    packet.flags = FlagResponse;
    packet.length = 6;
    packet.payload[0] = static_cast<std::uint8_t>(snapshot.mode);
    packet.payload[1] = snapshot.start_inhibit ? 1u : 0u;
    packet.payload[2] = snapshot.alarm_active ? 1u : 0u;
    packet.payload[3] = snapshot.shock_warning ? 1u : 0u;
    packet.payload[4] = snapshot.shock_trigger ? 1u : 0u;
    packet.payload[5] = snapshot.engine_running ? 1u : 0u;
    return node_.send(packet, now_ms, false);
}

bool SecurityServer::set_locked(bool locked, std::uint32_t now_ms) {
    if (!locked) {
        mode_ = SecurityMode::Unlocked;
        hardware_.set_alarm_output(false);
        hardware_.set_start_inhibit(false);
        publish_event(SecurityEventCode::Unlocked, last_controller_, now_ms);
        return true;
    }

    mode_ = SecurityMode::Locked;
    hardware_.set_alarm_output(false);
    update_start_inhibit();
    publish_event(SecurityEventCode::Locked, last_controller_, now_ms);
    return true;
}

bool SecurityServer::start_alarm(NodeAddress report_to, std::uint32_t now_ms) {
    if (mode_ == SecurityMode::Unlocked) return false;
    mode_ = SecurityMode::Alarm;
    if (!hardware_.set_alarm_output(true)) return false;
    update_start_inhibit();
    publish_event(SecurityEventCode::AlarmStarted, report_to, now_ms);
    return true;
}

bool SecurityServer::stop_alarm(NodeAddress report_to, std::uint32_t now_ms) {
    if (!hardware_.set_alarm_output(false)) return false;
    if (mode_ == SecurityMode::Alarm) mode_ = SecurityMode::Locked;
    update_start_inhibit();
    publish_event(SecurityEventCode::AlarmStopped, report_to, now_ms);
    return true;
}

bool SecurityServer::handle_command(SecurityCommand command, NodeAddress source, std::uint16_t sequence,
                                    bool ack_required, std::uint32_t now_ms) {
    last_controller_ = source;
    bool ok = false;

    switch (command) {
        case SecurityCommand::Lock:
            ok = set_locked(true, now_ms);
            break;
        case SecurityCommand::Unlock:
            ok = set_locked(false, now_ms);
            break;
        case SecurityCommand::SilenceAlarm:
            ok = stop_alarm(source, now_ms);
            break;
        case SecurityCommand::RequestState:
            ok = publish_state(source, now_ms);
            break;
        default:
            ok = false;
            break;
    }

    if (ack_required) node_.send_ack(source, sequence, !ok);
    if (ok && command != SecurityCommand::RequestState) publish_state(source, now_ms);
    return ok;
}

bool SecurityServer::handle_packet(const Packet& packet, std::uint32_t now_ms) {
    if (packet.type != MessageType::SecurityCommand) return false;
    if (packet.length != 1) {
        if ((packet.flags & FlagAckRequired) != 0) node_.send_ack(packet.source, packet.sequence, true);
        return true;
    }

    return handle_command(
        static_cast<SecurityCommand>(packet.payload[0]),
        packet.source,
        packet.sequence,
        (packet.flags & FlagAckRequired) != 0,
        now_ms);
}

void SecurityServer::service(std::uint32_t now_ms) {
    update_start_inhibit();

    const bool warning = hardware_.shock_warning_active();
    const bool trigger = hardware_.shock_trigger_active();

    if (mode_ != SecurityMode::Unlocked && warning && !previous_warning_) {
        publish_event(SecurityEventCode::ShockWarning, last_controller_, now_ms);
    }

    if (mode_ != SecurityMode::Unlocked && trigger && !previous_trigger_) {
        publish_event(SecurityEventCode::ShockTrigger, last_controller_, now_ms);
        start_alarm(last_controller_, now_ms);
    }

    previous_warning_ = warning;
    previous_trigger_ = trigger;
}

} // namespace bike
