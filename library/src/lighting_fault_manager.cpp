#include "bike/lighting_fault_manager.hpp"

namespace bike {

namespace {
constexpr LightingOutput kOutputs[] = {
    LightingOutput::LeftIndicator,
    LightingOutput::RightIndicator,
    LightingOutput::BrakeBright,
    LightingOutput::HighBeam
};

bool is_fault(LightingElectricalStatus status) {
    return status == LightingElectricalStatus::OpenLoad ||
           status == LightingElectricalStatus::OverCurrent;
}
}

std::size_t LightingFaultManager::index_for(LightingOutput output) {
    switch (output) {
        case LightingOutput::LeftIndicator:  return 0;
        case LightingOutput::RightIndicator: return 1;
        case LightingOutput::BrakeBright:    return 2;
        case LightingOutput::HighBeam:       return 3;
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

bool LightingFaultManager::clear_latch(LightingOutput output) {
    const auto index = index_for(output);
    if (index >= records_.size()) return false;

    auto& record = records_[index];
    record.latched = false;
    record.latched_status = LightingElectricalStatus::Off;
    record.shutdown_applied = false;
    consecutive_fault_samples_[index] = 0;
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

    if (consecutive_fault_samples_[index] < 0xFFu) {
        ++consecutive_fault_samples_[index];
    }

    if (consecutive_fault_samples_[index] < confirm_samples_) return;

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
        // Software shutdown is opt-in per channel. The default is ReportOnly,
        // so a missing/misbehaving sensor cannot silently remove safety lighting.
        if (outputs_.write_output(output, false)) {
            record.shutdown_applied = true;
        }
    }
}

void LightingFaultManager::service(std::uint32_t now_ms) {
    for (const auto output : kOutputs) {
        update_channel(output, now_ms);
    }
}

} // namespace bike
