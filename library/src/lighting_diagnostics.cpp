#include "bike/lighting_diagnostics.hpp"

namespace bike {

namespace {
constexpr LightingOutput kOutputs[] = {
    LightingOutput::LeftIndicator,
    LightingOutput::RightIndicator,
    LightingOutput::BrakeBright,
    LightingOutput::HighBeam
};
}

std::size_t LightingDiagnostics::index_for(LightingOutput output) {
    switch (output) {
        case LightingOutput::LeftIndicator:  return 0;
        case LightingOutput::RightIndicator: return 1;
        case LightingOutput::BrakeBright:    return 2;
        case LightingOutput::HighBeam:       return 3;
    }
    return 4;
}

void LightingDiagnostics::set_limits(LightingOutput output, LightingCurrentLimits limits) {
    const auto index = index_for(output);
    if (index < limits_.size()) limits_[index] = limits;
}

LightingChannelDiagnostic LightingDiagnostics::evaluate(LightingOutput output) const {
    LightingChannelDiagnostic result{};
    result.output = output;
    result.driven = outputs_.read_output(output);

    std::uint16_t current_ma = 0;
    result.feedback_available = feedback_.read_current_ma(output, current_ma);
    result.current_ma = current_ma;

    if (!result.driven) {
        result.status = LightingElectricalStatus::Off;
        return result;
    }

    if (!result.feedback_available) {
        result.status = LightingElectricalStatus::Unknown;
        return result;
    }

    const auto index = index_for(output);
    if (index >= limits_.size()) {
        result.status = LightingElectricalStatus::Unknown;
        return result;
    }

    const auto limits = limits_[index];
    if (current_ma < limits.open_load_below_ma) {
        result.status = LightingElectricalStatus::OpenLoad;
    } else if (current_ma > limits.over_current_above_ma) {
        result.status = LightingElectricalStatus::OverCurrent;
    } else {
        result.status = LightingElectricalStatus::Ok;
    }

    return result;
}

bool LightingDiagnostics::publish(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::LightingDiagnostic;
    packet.flags = FlagResponse;
    packet.length = 24;

    bool any_fault = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto diagnostic = evaluate(kOutputs[i]);
        const auto offset = i * 6;
        packet.payload[offset] = static_cast<std::uint8_t>(diagnostic.output);
        packet.payload[offset + 1] = diagnostic.driven ? 1u : 0u;
        packet.payload[offset + 2] = diagnostic.feedback_available ? 1u : 0u;
        packet.payload[offset + 3] = static_cast<std::uint8_t>(diagnostic.status);
        packet.payload[offset + 4] = static_cast<std::uint8_t>((diagnostic.current_ma >> 8) & 0xFFu);
        packet.payload[offset + 5] = static_cast<std::uint8_t>(diagnostic.current_ma & 0xFFu);

        if (diagnostic.status == LightingElectricalStatus::OpenLoad ||
            diagnostic.status == LightingElectricalStatus::OverCurrent) {
            any_fault = true;
        }
    }

    if (any_fault) packet.flags |= FlagFault;
    const bool sent = node_.send(packet, now_ms, false);
    if (sent) {
        last_publish_ms_ = now_ms;
        published_once_ = true;
    }
    return sent;
}

void LightingDiagnostics::service(std::uint32_t now_ms, NodeAddress destination) {
    if (!published_once_ ||
        static_cast<std::uint32_t>(now_ms - last_publish_ms_) >= publish_interval_ms_) {
        publish(destination, now_ms);
    }
}

} // namespace bike
