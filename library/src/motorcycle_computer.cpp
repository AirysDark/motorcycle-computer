#include "bike/motorcycle_computer.hpp"

namespace bike {

namespace {
std::uint32_t read_u32(const Packet& packet, std::size_t offset) {
    return (static_cast<std::uint32_t>(packet.payload[offset]) << 24) |
           (static_cast<std::uint32_t>(packet.payload[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(packet.payload[offset + 2]) << 8) |
            static_cast<std::uint32_t>(packet.payload[offset + 3]);
}
}

std::size_t MotorcycleComputer::lighting_index(LightingOutput output) {
    switch (output) {
        case LightingOutput::LeftIndicator: return 0;
        case LightingOutput::RightIndicator: return 1;
        case LightingOutput::BrakeBright: return 2;
        case LightingOutput::HighBeam: return 3;
    }
    return 4;
}

void MotorcycleComputer::begin(std::uint32_t now_ms) {
    supervisor_.send_discovery(now_ms);
    supervisor_.send_heartbeat(now_ms);
    request_all_states(now_ms);
}

bool MotorcycleComputer::decode_lighting_state(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Lighting || packet.type != MessageType::StateUpdate || packet.length != 8) return false;
    LightingSnapshot next = lighting_state_;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto raw_output = packet.payload[i * 2];
        const auto raw_value = packet.payload[i * 2 + 1];
        if (raw_value > 1u) return false;
        const bool value = raw_value != 0;
        switch (static_cast<LightingOutput>(raw_output)) {
            case LightingOutput::LeftIndicator: next.left_indicator = value; break;
            case LightingOutput::RightIndicator: next.right_indicator = value; break;
            case LightingOutput::BrakeBright: next.brake_bright = value; break;
            case LightingOutput::HighBeam: next.high_beam = value; break;
            default: return false;
        }
    }
    next.valid = true;
    next.updated_at_ms = now_ms;
    lighting_state_ = next;
    return true;
}

bool MotorcycleComputer::decode_lighting_diagnostics(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Lighting || packet.type != MessageType::LightingDiagnostic || packet.length != 24) return false;
    LightingDiagnosticSnapshot next{};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto offset = i * 6;
        const auto raw_output = packet.payload[offset];
        const auto raw_driven = packet.payload[offset + 1];
        const auto raw_feedback = packet.payload[offset + 2];
        const auto raw_status = packet.payload[offset + 3];
        if (raw_driven > 1u || raw_feedback > 1u || raw_status > static_cast<std::uint8_t>(LightingElectricalStatus::OverCurrent)) return false;
        const auto output = static_cast<LightingOutput>(raw_output);
        if (lighting_index(output) >= 4) return false;
        auto& channel = next.channels[i];
        channel.output = output;
        channel.driven = raw_driven != 0;
        channel.feedback_available = raw_feedback != 0;
        channel.status = static_cast<LightingElectricalStatus>(raw_status);
        channel.current_ma = static_cast<std::uint16_t>((static_cast<std::uint16_t>(packet.payload[offset + 4]) << 8) | packet.payload[offset + 5]);
    }
    next.valid = true;
    next.updated_at_ms = now_ms;
    lighting_diagnostics_ = next;
    return true;
}

bool MotorcycleComputer::decode_lighting_fault(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Lighting ||
        (packet.type != MessageType::Fault && packet.type != MessageType::FaultClear) ||
        packet.length != 8 || packet.payload[0] != 0x01) return false;
    const auto output = static_cast<LightingOutput>(packet.payload[1]);
    const auto index = lighting_index(output);
    if (index >= lighting_faults_.size()) return false;
    const auto raw_status = packet.payload[2];
    if (raw_status > static_cast<std::uint8_t>(LightingElectricalStatus::OverCurrent) || packet.payload[3] > 1u) return false;
    auto& fault = lighting_faults_[index];
    fault.latched = packet.type == MessageType::Fault;
    fault.status = fault.latched ? static_cast<LightingElectricalStatus>(raw_status) : LightingElectricalStatus::Off;
    fault.shutdown_applied = fault.latched && packet.payload[3] != 0;
    fault.occurrence_count = read_u32(packet, 4);
    fault.updated_at_ms = now_ms;
    return true;
}

bool MotorcycleComputer::decode_northbridge_diagnostics(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Northbridge || packet.type != MessageType::Diagnostic ||
        packet.length < 18 || packet.payload[0] != 0x02) return false;
    const auto port_count = packet.payload[1];
    if (port_count > kMaxRouterPorts || packet.length != static_cast<std::uint16_t>(18 + port_count * 21)) return false;

    NorthbridgeSnapshot next = northbridge_state_;
    next.forwarded_packets = read_u32(packet, 2);
    next.dropped_packets = read_u32(packet, 6);
    next.route_movement_events = read_u32(packet, 10);
    next.topology_fault_events = read_u32(packet, 14);
    for (auto& port : next.ports) port = NorthbridgePortSnapshot{};

    std::size_t offset = 18;
    for (std::size_t i = 0; i < port_count; ++i) {
        const auto port = packet.payload[offset];
        if (port >= kMaxRouterPorts) return false;
        auto& stats = next.ports[port];
        stats.attached = true;
        stats.rx_packets = read_u32(packet, offset + 1);
        stats.tx_packets = read_u32(packet, offset + 5);
        stats.rx_bytes = read_u32(packet, offset + 9);
        stats.dropped_packets = read_u32(packet, offset + 13);
        stats.malformed_frames = read_u32(packet, offset + 17);
        offset += 21;
    }
    next.valid = true;
    next.updated_at_ms = now_ms;
    northbridge_state_ = next;
    return true;
}

bool MotorcycleComputer::decode_northbridge_fault(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Northbridge || packet.type != MessageType::Fault ||
        packet.length != 9 || packet.payload[0] != 0x02 || packet.payload[1] != 0x01) return false;
    northbridge_state_.topology_fault_active = true;
    northbridge_state_.topology_fault_node = static_cast<NodeAddress>(packet.payload[2]);
    northbridge_state_.expected_port = packet.payload[3];
    northbridge_state_.actual_port = packet.payload[4];
    northbridge_state_.topology_fault_events = read_u32(packet, 5);
    northbridge_state_.updated_at_ms = now_ms;
    return true;
}

bool MotorcycleComputer::decode_security_state(const Packet& packet, std::uint32_t now_ms) {
    if (packet.source != NodeAddress::Security || packet.type != MessageType::SecurityState || packet.length != 6) return false;
    if (packet.payload[0] > static_cast<std::uint8_t>(SecurityMode::LockPending)) return false;
    for (std::size_t i = 1; i < 6; ++i) if (packet.payload[i] > 1u) return false;
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
    if (packet.source != NodeAddress::Security || packet.type != MessageType::SecurityEvent || packet.length != 1) return false;
    const auto raw = packet.payload[0];
    if (raw < static_cast<std::uint8_t>(SecurityEventCode::ShockWarning) || raw > static_cast<std::uint8_t>(SecurityEventCode::LockPending)) return false;
    security_state_.last_event = static_cast<SecurityEventCode>(raw);
    security_state_.has_event = true;
    security_state_.last_event_at_ms = now_ms;
    return true;
}

void MotorcycleComputer::dispatch(const Packet& packet, std::uint32_t now_ms) {
    supervisor_.observe(packet, now_ms);
    if (decode_lighting_state(packet, now_ms)) return;
    if (decode_lighting_diagnostics(packet, now_ms)) return;
    if (decode_lighting_fault(packet, now_ms)) return;
    if (decode_northbridge_diagnostics(packet, now_ms)) return;
    if (decode_northbridge_fault(packet, now_ms)) return;
    if (decode_security_state(packet, now_ms)) return;
    decode_security_event(packet, now_ms);
}

void MotorcycleComputer::service(std::uint32_t now_ms) {
    Packet packet{};
    while (node_.poll(packet, now_ms)) dispatch(packet, now_ms);
    node_.service(now_ms);
    supervisor_.service(now_ms);
}

MotorcycleSnapshot MotorcycleComputer::snapshot() const {
    MotorcycleSnapshot result{};
    result.lighting_node = supervisor_.status(NodeAddress::Lighting);
    result.security_node = supervisor_.status(NodeAddress::Security);
    result.northbridge_node = supervisor_.status(NodeAddress::Northbridge);
    result.lighting = lighting_state_;
    result.lighting_diagnostics = lighting_diagnostics_;
    result.lighting_faults = lighting_faults_;
    result.northbridge = northbridge_state_;
    result.security = security_state_;
    result.tx_failures = node_.tx_failures();
    result.rx_drops = node_.rx_drops();
    return result;
}

bool MotorcycleComputer::request_all_states(std::uint32_t now_ms) {
    Packet lighting_request{};
    lighting_request.destination = NodeAddress::Lighting;
    lighting_request.type = MessageType::GetState;
    const bool lighting_ok = node_.send(lighting_request, now_ms, false);

    Packet northbridge_request{};
    northbridge_request.destination = NodeAddress::Northbridge;
    northbridge_request.type = MessageType::Diagnostic;
    const bool northbridge_ok = node_.send(northbridge_request, now_ms, false);

    const bool security_ok = security_.request_state(now_ms);
    return lighting_ok && northbridge_ok && security_ok;
}

} // namespace bike
