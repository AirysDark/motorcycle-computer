#include "bike/motorcycle_computer.hpp"

namespace bike {

void MotorcycleComputer::begin(std::uint32_t now_ms) {
    supervisor_.send_discovery(now_ms);
    supervisor_.send_heartbeat(now_ms);
    request_all_states(now_ms);
}

void MotorcycleComputer::dispatch(const Packet& packet, std::uint32_t now_ms) {
    supervisor_.observe(packet, now_ms);
    // Typed subsystem state decoders will be added as state schemas expand.
    // For now all received packets contribute to node health/supervision.
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
    result.lighting = supervisor_.status(NodeAddress::Lighting);
    result.security = supervisor_.status(NodeAddress::Security);
    result.northbridge = supervisor_.status(NodeAddress::Northbridge);
    result.tx_failures = node_.tx_failures();
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
