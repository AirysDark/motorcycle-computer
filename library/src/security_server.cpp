#include "bike/security_server.hpp"

namespace bike {

SecurityStateSnapshot SecurityServer::state() const {
    SecurityStateSnapshot snapshot{};
    snapshot.mode = mode_;
    snapshot.start_inhibit = hardware_.start_inhibit_active();
    snapshot.alarm_active = hardware_.alarm_output_active();
    snapshot.shock_warning = filtered_warning_;
    snapshot.shock_trigger = filtered_trigger_;
    snapshot.engine_running = engine_filter_initialized_ ? filtered_engine_running_ : hardware_.engine_running();
    return snapshot;
}

void SecurityServer::persist_armed(bool armed) {
    if (persistence_ != nullptr) persistence_->save_armed(armed);
}

bool SecurityServer::restore_persisted_state(std::uint32_t now_ms) {
    hardware_.set_alarm_output(false);
    hardware_.set_start_inhibit(false);
    alarm_timer_active_ = false;

    bool armed = false;
    if (persistence_ == nullptr || !persistence_->load_armed(armed) || !armed) {
        mode_ = SecurityMode::Unlocked;
        recovery_requires_stop_confirmation_ = false;
        return false;
    }

    // A restored armed intent is never trusted as immediately safe to inhibit.
    // Pretend the engine is running until the physical input has been observed
    // stopped continuously for the normal confirmation interval.
    mode_ = SecurityMode::LockPending;
    filtered_engine_running_ = true;
    engine_filter_initialized_ = true;
    engine_stop_pending_ = false;
    engine_stop_started_ms_ = now_ms;
    recovery_requires_stop_confirmation_ = true;
    previous_warning_ = false;
    previous_trigger_ = false;
    return true;
}

bool SecurityServer::update_filtered_assert(bool raw, bool& stable, bool& pending,
                                            std::uint32_t& changed_at_ms,
                                            std::uint32_t confirm_ms,
                                            std::uint32_t now_ms) {
    if (!raw) {
        pending = false;
        const bool changed = stable;
        stable = false;
        return changed;
    }

    if (stable) {
        pending = false;
        return false;
    }

    if (!pending) {
        pending = true;
        changed_at_ms = now_ms;
        if (confirm_ms != 0) return false;
    }

    if (static_cast<std::uint32_t>(now_ms - changed_at_ms) < confirm_ms) return false;
    stable = true;
    pending = false;
    return true;
}

void SecurityServer::sample_inputs(std::uint32_t now_ms) {
    const bool raw_engine = hardware_.engine_running();
    if (!engine_filter_initialized_) {
        filtered_engine_running_ = raw_engine;
        engine_filter_initialized_ = true;
    } else if (raw_engine) {
        // Assert running immediately. Any ambiguity must keep start prevention OFF.
        filtered_engine_running_ = true;
        engine_stop_pending_ = false;
    } else if (filtered_engine_running_) {
        if (!engine_stop_pending_) {
            engine_stop_pending_ = true;
            engine_stop_started_ms_ = now_ms;
        }
        if (static_cast<std::uint32_t>(now_ms - engine_stop_started_ms_) >= engine_stop_confirm_ms_) {
            filtered_engine_running_ = false;
            engine_stop_pending_ = false;
        }
    } else {
        engine_stop_pending_ = false;
    }

    update_filtered_assert(hardware_.shock_warning_active(), filtered_warning_, warning_pending_,
                           warning_changed_at_ms_, warning_confirm_ms_, now_ms);
    update_filtered_assert(hardware_.shock_trigger_active(), filtered_trigger_, trigger_pending_,
                           trigger_changed_at_ms_, trigger_confirm_ms_, now_ms);
}

void SecurityServer::update_start_inhibit() {
    const bool engine_running = engine_filter_initialized_ ? filtered_engine_running_ : hardware_.engine_running();
    const bool should_inhibit =
        (mode_ == SecurityMode::Locked || mode_ == SecurityMode::Alarm) && !engine_running;
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
    sample_inputs(now_ms);

    if (!locked) {
        mode_ = SecurityMode::Unlocked;
        hardware_.set_alarm_output(false);
        hardware_.set_start_inhibit(false);
        alarm_timer_active_ = false;
        recovery_requires_stop_confirmation_ = false;
        persist_armed(false);
        publish_event(SecurityEventCode::Unlocked, last_controller_, now_ms);
        return true;
    }

    hardware_.set_alarm_output(false);
    alarm_timer_active_ = false;
    recovery_requires_stop_confirmation_ = false;
    persist_armed(true);
    if (filtered_engine_running_) {
        mode_ = SecurityMode::LockPending;
        publish_event(SecurityEventCode::LockPending, last_controller_, now_ms);
    } else {
        mode_ = SecurityMode::Locked;
        publish_event(SecurityEventCode::Locked, last_controller_, now_ms);
    }
    update_start_inhibit();
    return true;
}

bool SecurityServer::start_alarm(NodeAddress report_to, std::uint32_t now_ms) {
    if (mode_ != SecurityMode::Locked) return false;
    if (!hardware_.set_alarm_output(true)) return false;
    mode_ = SecurityMode::Alarm;
    alarm_started_ms_ = now_ms;
    alarm_timer_active_ = true;
    update_start_inhibit();
    publish_event(SecurityEventCode::AlarmStarted, report_to, now_ms);
    return true;
}

bool SecurityServer::stop_alarm(NodeAddress report_to, std::uint32_t now_ms) {
    if (!hardware_.set_alarm_output(false)) return false;
    alarm_timer_active_ = false;
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
            sample_inputs(now_ms);
            update_start_inhibit();
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
    sample_inputs(now_ms);

    if (mode_ == SecurityMode::LockPending && !filtered_engine_running_) {
        mode_ = SecurityMode::Locked;
        recovery_requires_stop_confirmation_ = false;
        publish_event(SecurityEventCode::Locked, last_controller_, now_ms);
        publish_state(last_controller_, now_ms);
    }

    update_start_inhibit();

    if (mode_ == SecurityMode::Alarm && alarm_timer_active_ &&
        static_cast<std::uint32_t>(now_ms - alarm_started_ms_) >= alarm_duration_ms_) {
        stop_alarm(last_controller_, now_ms);
        publish_state(last_controller_, now_ms);
    }

    if (mode_ == SecurityMode::Locked && filtered_warning_ && !previous_warning_) {
        publish_event(SecurityEventCode::ShockWarning, last_controller_, now_ms);
    }

    if (mode_ == SecurityMode::Locked && filtered_trigger_ && !previous_trigger_) {
        publish_event(SecurityEventCode::ShockTrigger, last_controller_, now_ms);
        if (start_alarm(last_controller_, now_ms)) publish_state(last_controller_, now_ms);
    }

    previous_warning_ = filtered_warning_;
    previous_trigger_ = filtered_trigger_;
}

} // namespace bike
