#pragma once
#include <cstdint>
#include "bike/lighting.hpp"
#include "bike/security.hpp"
#include "bike/supervisor.hpp"

namespace bike {

struct MotorcycleSnapshot {
    const NodeStatus* lighting{nullptr};
    const NodeStatus* security{nullptr};
    const NodeStatus* northbridge{nullptr};
    std::uint32_t tx_failures{0};
};

class MotorcycleComputer {
public:
    MotorcycleComputer(BikeNode& node)
        : node_(node), lighting_(node), security_(node), supervisor_(node) {}

    LightingClient& lighting() { return lighting_; }
    SecurityClient& security() { return security_; }
    NetworkSupervisor& supervisor() { return supervisor_; }

    void begin(std::uint32_t now_ms);
    void service(std::uint32_t now_ms);
    MotorcycleSnapshot snapshot() const;

    bool request_all_states(std::uint32_t now_ms);

private:
    void dispatch(const Packet& packet, std::uint32_t now_ms);

    BikeNode& node_;
    LightingClient lighting_;
    SecurityClient security_;
    NetworkSupervisor supervisor_;
};

} // namespace bike
