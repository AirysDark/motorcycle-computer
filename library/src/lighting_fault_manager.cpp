#include "bike/lighting_fault_manager.hpp"

namespace bike {

namespace {
constexpr LightingOutput kOutputs[] = {
    LightingOutput::LeftIndicator, LightingOutput::RightIndicator,
    LightingOutput::BrakeBright, LightingOutput::HighBeam
};
bool is_fault(LightingElectricalStatus status) {
    return status == LightingElectricalStatus::OpenLoad || status == LightingElectricalStatus::OverCurrent;
}
}

std::size_t LightingFaultManager::index_for(LightingOutput output) {
    switch (output) {
        case LightingOutput::LeftIndicator: return 0;
        case LightingOutput::RightIndicator: return 1;
        case LightingOutput::BrakeBright: return 2;
        case LightingOutput::HighBeam: return 3;
    }
    return 4;
}

void LightingFaultManager::set_policy(LightingOutput output, LightingFaultPolicy policy) {
    const auto index = index_for(output);
    if (index < policies_.size()) policies_[index] = policy;
}

const LightingFaultRecord& LightingFaultManager::record(LightingOutput output) const {
    static const LightingFaultRecord kInvalid{};
    const auto index = index_for(output);
    return index < records_.size() ? records_[index] : kInvalid;
}

bool LightingFaultManager::publish_fault(LightingOutput output, const LightingFaultRecord& record,
                                         MessageType type, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = NodeAddress::MainComputer;
    packet.type = type;
    packet.flags = FlagResponse;
    if (type == MessageType::Fault) packet.flags |= FlagFault;
    packet.length = 8;
    packet.payload[0] = 0x01; // subsystem: lighting
    packet.payload[1] = static_cast<std::uint8_t>(output);
    packet.payload[2] = static_cast<std::uint8_t>(record.latched_status);
    packet.payload[3] = record.shutdown_applied ? 1u : 0u;
    packet.payload[4] = static_cast<std::uint8_t>((record.occurrence_count >> 24) & 0xFFu);
    packet.payload[5] = static_cast<std::uint8_t>((record.occurrence_count >> 16) & 0xFFu);
    packet.payload[6] = static_cast<std::uint8_t>((record.occurrence_count >> 8) & 0xFFu);
    packet.payload[7] = static_cast<std::uint8_t>(record.occurrence_count & 0xFFu);
    return node_.send(packet, now_ms, false);
}

bool LightingFaultManager::clear_latch(LightingOutput output, std::uint32_t now_ms) {
    const auto index = index_for(output);
    if (index >= records_.size()) return false;
    auto previous = records_[index];
    records_[index].latched = false;
    records_[index].latched_status = LightingElectricalStatus::Off;
    records_[index].shutdown_applied = false;
    consecutive_fault_samples_[index] = 0;
    if (previous.latched) publish_fault(output, previous, MessageType::FaultClear, now_ms);
    return true;
}

void LightingFaultManager::update_channel(LightingOutput output, std::uint32_t now_ms) {
    const auto index = index_for(output);
    if (index >= records_.size()) return;
    const auto diagnostic = diagnostics_.evaluate(output);
    auto& record = records_[index];
    record.active_status = diagnostic.status;
    record.last_current_ma = diagnostic.current_ma;

    if (!is_fault(diagnostic.status)) {
        consecutive_fault_samples_[index] = 0;
        return;
    }
    if (consecutive_fault_samples_[index] < 0xFFu) ++consecutive_fault_samples_[index];
    if (consecutive_fault_samples_[index] < confirm_samples_) return;

    const bool newly_latched = !record.latched || record.latched_status != diagnostic.status;
    if (!record.latched) {
        record.first_seen_ms = now_ms;
        record.latched = true;
        ++record.occurrence_count;
    } else if (record.latched_status != diagnostic.status) {
        ++record.occurrence_count;
    }
    record.latched_status = diagnostic.status;
    record.last_seen_ms = now_ms;

    if (diagnostic.status == LightingElectricalStatus::OverCurrent &&
        policies_[index] == LightingFaultPolicy::ShutdownOnOverCurrent) {
        if (outputs_.write_output(output, false)) record.shutdown_applied = true;
    }
    if (newly_latched) publish_fault(output, record, MessageType::Fault, now_ms);
}

void LightingFaultManager::service(std::uint32_t now_ms) {
    for (const auto output : kOutputs) update_channel(output, now_ms);
}

} // namespace bike
