#pragma once
#include <array>
#include <cstdint>
#include "bike/lighting_diagnostics.hpp"

namespace bike {

enum class LightingFaultPolicy : std::uint8_t { ReportOnly = 0, ShutdownOnOverCurrent = 1 };

struct LightingFaultRecord {
    LightingElectricalStatus active_status{LightingElectricalStatus::Off};
    LightingElectricalStatus latched_status{LightingElectricalStatus::Off};
    std::uint32_t first_seen_ms{0};
    std::uint32_t last_seen_ms{0};
    std::uint32_t occurrence_count{0};
    std::uint16_t last_current_ma{0};
    bool latched{false};
    bool shutdown_applied{false};
};

class LightingFaultManager {
public:
    LightingFaultManager(BikeNode& node, LightingDiagnostics& diagnostics, LightingHardware& outputs)
        : node_(node), diagnostics_(diagnostics), outputs_(outputs) {}

    void set_policy(LightingOutput output, LightingFaultPolicy policy);
    void set_confirm_samples(std::uint8_t value) { confirm_samples_ = value == 0 ? 1 : value; }
    void service(std::uint32_t now_ms);
    bool clear_latch(LightingOutput output, std::uint32_t now_ms = 0);
    const LightingFaultRecord& record(LightingOutput output) const;

private:
    static std::size_t index_for(LightingOutput output);
    void update_channel(LightingOutput output, std::uint32_t now_ms);
    bool publish_fault(LightingOutput output, const LightingFaultRecord& record,
                       MessageType type, std::uint32_t now_ms);

    BikeNode& node_;
    LightingDiagnostics& diagnostics_;
    LightingHardware& outputs_;
    std::array<LightingFaultPolicy, 4> policies_{};
    std::array<LightingFaultRecord, 4> records_{};
    std::array<std::uint8_t, 4> consecutive_fault_samples_{};
    std::uint8_t confirm_samples_{3};
};

} // namespace bike
