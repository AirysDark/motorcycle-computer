#include "bike/northbridge.hpp"

namespace bike {

bool NorthbridgeRuntime::attach_port(std::uint8_t port, Transport& transport) {
    if (port >= transports_.size()) return false;
    transports_[port] = &transport;
    return router_.attach_port(port, transport);
}

void NorthbridgeRuntime::service_port(std::uint8_t port, std::uint32_t now_ms) {
    if (port >= transports_.size() || transports_[port] == nullptr) return;

    std::array<std::uint8_t, 64> chunk{};
    const auto count = transports_[port]->read(chunk.data(), chunk.size());

    for (std::size_t i = 0; i < count; ++i) {
        Packet packet{};
        if (!parsers_[port].push(chunk[i], packet)) continue;

        if (router_.forward(packet, port, now_ms)) {
            ++forwarded_packets_;
        } else {
            ++dropped_packets_;
        }
    }
}

void NorthbridgeRuntime::service(std::uint32_t now_ms) {
    for (std::uint8_t port = 0; port < transports_.size(); ++port) {
        service_port(port, now_ms);
    }
}

} // namespace bike
