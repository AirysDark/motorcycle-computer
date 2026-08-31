#include "bike/lighting_server.hpp"

namespace bike {

namespace {

bool state_value(const LightingState& state, LightingOutput output) {
    switch (output) {
        case LightingOutput::LeftIndicator:  return state.left_indicator;
        case LightingOutput::RightIndicator: return state.right_indicator;
        case LightingOutput::BrakeBright:    return state.brake_bright;
        case LightingOutput::HighBeam:       return state.high_beam;
    }
    return false;
}

} // namespace

bool LightingServer::valid_output(std::uint8_t raw) const {
    switch (static_cast<LightingOutput>(raw)) {
        case LightingOutput::LeftIndicator:
        case LightingOutput::RightIndicator:
        case LightingOutput::BrakeBright:
        case LightingOutput::HighBeam:
            return true;
    }
    return false;
}

void LightingServer::set_commanded(LightingOutput output, bool enabled) {
    switch (output) {
        case LightingOutput::LeftIndicator:  commanded_.left_indicator = enabled; break;
        case LightingOutput::RightIndicator: commanded_.right_indicator = enabled; break;
        case LightingOutput::BrakeBright:    commanded_.brake_bright = enabled; break;
        case LightingOutput::HighBeam:       commanded_.high_beam = enabled; break;
    }
}

bool LightingServer::apply_output(LightingOutput output, bool enabled) {
    if (!hardware_.write_output(output, enabled)) return false;
    set_commanded(output, enabled);
    return true;
}

LightingState LightingServer::actual_state() const {
    LightingState state{};
    state.left_indicator = hardware_.read_output(LightingOutput::LeftIndicator);
    state.right_indicator = hardware_.read_output(LightingOutput::RightIndicator);
    state.brake_bright = hardware_.read_output(LightingOutput::BrakeBright);
    state.high_beam = hardware_.read_output(LightingOutput::HighBeam);
    return state;
}

bool LightingServer::publish_state(NodeAddress destination, std::uint32_t now_ms) {
    const auto actual = actual_state();
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::StateUpdate;
    packet.length = 8;

    const LightingOutput outputs[] = {
        LightingOutput::LeftIndicator,
        LightingOutput::RightIndicator,
        LightingOutput::BrakeBright,
        LightingOutput::HighBeam
    };

    for (std::size_t i = 0; i < 4; ++i) {
        packet.payload[i * 2] = static_cast<std::uint8_t>(outputs[i]);
        packet.payload[i * 2 + 1] = state_value(actual, outputs[i]) ? 1u : 0u;
    }

    return node_.send(packet, now_ms, false);
}

bool LightingServer::handle_packet(const Packet& packet, std::uint32_t now_ms) {
    if (packet.type != MessageType::SetOutput) return false;

    if (packet.length != 2 || !valid_output(packet.payload[0]) || packet.payload[1] > 1u) {
        node_.send_ack(packet.source, packet.sequence, true);
        return true;
    }

    const auto output = static_cast<LightingOutput>(packet.payload[0]);
    const bool enabled = packet.payload[1] != 0;

    if (!apply_output(output, enabled)) {
        node_.send_ack(packet.source, packet.sequence, true);
        return true;
    }

    node_.send_ack(packet.source, packet.sequence, false);
    publish_state(packet.source, now_ms);
    return true;
}

} // namespace bike
