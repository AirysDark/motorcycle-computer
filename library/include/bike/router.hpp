#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/transport.hpp"
#include "bike_protocol/codec.hpp"

namespace bike {

constexpr std::size_t kMaxRouterPorts = 8;

struct RouteEntry {
    NodeAddress address{NodeAddress::Broadcast};
    std::uint8_t port{0};
    bool active{false};
    std::uint32_t last_seen_ms{0};
};

class NorthbridgeRouter {
public:
    bool attach_port(std::uint8_t port, Transport& transport);
    bool set_route(NodeAddress address, std::uint8_t port);
    bool remove_route(NodeAddress address);
    int route_for(NodeAddress address) const;

    bool forward(const Packet& packet, std::uint8_t ingress_port, std::uint32_t now_ms);
    void mark_seen(NodeAddress address, std::uint8_t ingress_port, std::uint32_t now_ms);
    bool is_online(NodeAddress address, std::uint32_t now_ms, std::uint32_t timeout_ms) const;

private:
    bool write_to_port(std::uint8_t port, const Packet& packet);

    std::array<Transport*, kMaxRouterPorts> ports_{};
    std::array<RouteEntry, kMaxRouterPorts> routes_{};
};

} // namespace bike
