#include "bike/motorcycle_computer.hpp"

namespace bike {

void MotorcycleComputer::begin(std::uint32_t now_ms) {
    supervisor_.send_discovery(now_ms);
    supervisor_.send_heartbeat(now_ms);
    request_all_states(now_ms);
}

bool MotorcycleComputer::decode_lighting_state(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Lighting || packet.type != MessageType::StateUpdate || packet.length != 8) {
        return false;
    }

    LightingSnapshot next = lighting_state_;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto raw_output = packet.payload[i * 2];
        const auto raw_value = packet.payload[i * 2 + 1];
        if (raw_value > 1u) return false;
        const bool value = raw_value != 0;

        switch (static_cast<LightingOutput>(raw_output)) {
            case LightingOutput::LeftIndicator:  next.left_indicator = value; break;
            case LightingOutput::RightIndicator: next.right_indicator = value; break;
            case LightingOutput::BrakeBright:    next.brake_bright = value; break;
            case LightingOutput::HighBeam:       next.high_beam = value; break;
            default: return false;
        }
    }

    next.valid = true;
    next.updated_at_ms = now_ms;
    lighting_state_ = next;
    return true;
}

bool MotorcycleComputer::decode_security_state(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Security || packet.type != MessageType::SecurityState || packet.length != 6) {
        return false;
    }

    if (packet.payload[0] > static_cast<std::uint8_t>(SecurityMode::Alarm)) return false;
    for (std::size_t i = 1; i < 6; ++i) {
        if (packet.payload[i] > 1u) return false;
    }

    security_state_.mode = static_cast<SecurityMode>(packet.payload[0]);
    security_state_.start_inhibit = packet.payload[1] != 0;
    security_state_.alarm_active = packet.payload[2] != 0;
    security_state_.shock_warning = packet.payload[3] != 0;
    security_state_.shock_trigger = packet.payload[4] != 0;
    security_state_.engine_running = packet.payload[5] != 0;
    security_state_.valid = true;
    security_state_.updated_at_ms = now_ms;
    return true;
}

bool MotorcycleComputer::decode_security_event(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Security || packet.type != MessageType::SecurityEvent || packet.length != 1) {
        return false;
    }

    const auto raw = packet.payload[0];
    if (raw < static_cast<std::uint8_t>(SecurityEventCode::ShockWarning) ||
        raw > static_cast<std::uint8_t>(SecurityEventCode::Unlocked)) {
        return false;
    }

    security_state_.last_event = static_cast<SecurityEventCode>(raw);
    security_state_.has_event = true;
    security_state_.last_event_at_ms = now_ms;
    return true;
}

void MotorcycleComputer::dispatch(const Packet& packet, std::uint32_t now_ms) {
    supervisor_.observe(packet, now_ms);
    if (decode_lighting_state(packet, now_ms)) return;
    if (decode_security_state(packet, now_ms)) return;
    decode_security_event(packet, now_ms);
}

void MotorcycleComputer::service(std::uint32_t now_ms) {
    Packet packet{};
    while (node_.poll(packet, now_ms)) {
        dispatch(packet, now_ms);
    }

    node_.service(now_ms);
    supervisor_.service(now_ms);
}

MotorcycleSnapshot MotorcycleComputer::snapshot() const {
    MotorcycleSnapshot result{};
    result.lighting_node = supervisor_.status(NodeAddress::Lighting);
    result.security_node = supervisor_.status(NodeAddress::Security);
    result.northbridge_node = supervisor_.status(NodeAddress::Northbridge);
    result.lighting = lighting_state_;
    result.security = security_state_;
    result.tx_failures = node_.tx_failures();
    result.rx_drops = node_.rx_drops();
    return result;
}

bool MotorcycleComputer::request_all_states(std::uint32_t now_ms) {
    Packet lighting_request{};
    lighting_request.destination = NodeAddress::Lighting;
    lighting_request.type = MessageType::GetState;
    lighting_request.length = 0;

    const bool lighting_ok = node_.send(lighting_request, now_ms, false);
    const bool security_ok = security_.request_state(now_ms);
    return lighting_ok && security_ok;
}

} // namespace bike
