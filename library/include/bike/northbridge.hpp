#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/router.hpp"
#include "bike/stream_parser.hpp"
#include "bike/transport.hpp"

namespace bike {

struct NorthbridgePortStats {
    std::uint32_t rx_bytes{0};
    std::uint32_t rx_packets{0};
    std::uint32_t tx_packets{0};
    std::uint32_t dropped_packets{0};
    std::uint32_t malformed_frames{0};
};

struct TopologyBinding {
    NodeAddress address{NodeAddress::Broadcast};
    std::uint8_t port{0};
    bool active{false};
};

class NorthbridgeRuntime {
public:
    bool attach_port(std::uint8_t port, Transport& transport);
    bool set_route(NodeAddress address, std::uint8_t port) { return router_.set_route(address, port); }
    bool bind_node(NodeAddress address, std::uint8_t port);
    void set_topology_enforced(bool enabled) { topology_enforced_ = enabled; }

    void service(std::uint32_t now_ms);
    std::uint32_t forwarded_packets() const { return forwarded_packets_; }
    std::uint32_t dropped_packets() const { return dropped_packets_; }
    std::uint32_t route_movement_events() const { return route_movement_events_; }
    std::uint32_t topology_fault_events() const { return topology_fault_events_; }
    const NorthbridgePortStats& port_stats(std::uint8_t port) const;

    NorthbridgeRouter& router() { return router_; }
    const NorthbridgeRouter& router() const { return router_; }

private:
    void service_port(std::uint8_t port, std::uint32_t now_ms);
    bool handle_local_packet(const Packet& packet, std::uint8_t ingress_port, std::uint32_t now_ms);
    bool send_local(Packet packet, std::uint8_t egress_port);
    bool send_device_info(NodeAddress destination, std::uint8_t egress_port);
    bool send_heartbeat(NodeAddress destination, std::uint8_t egress_port);
    bool send_diagnostics(NodeAddress destination, std::uint8_t egress_port);
    bool send_topology_fault(NodeAddress address, std::uint8_t expected_port, std::uint8_t actual_port);
    void account_forward(const Packet& packet, std::uint8_t ingress_port, bool success);
    int expected_port_for(NodeAddress address) const;

    NorthbridgeRouter router_{};
    std::array<Transport*, kMaxRouterPorts> transports_{};
    std::array<StreamParser, kMaxRouterPorts> parsers_{};
    std::array<NorthbridgePortStats, kMaxRouterPorts> port_stats_{};
    std::array<TopologyBinding, kMaxRouterPorts> bindings_{};
    std::array<std::uint32_t, kMaxRouterPorts> parser_reject_snapshot_{};
    std::uint32_t forwarded_packets_{0};
    std::uint32_t dropped_packets_{0};
    std::uint32_t route_movement_events_{0};
    std::uint32_t topology_fault_events_{0};
    std::uint16_t local_sequence_{1};
    bool topology_enforced_{false};
};

} // namespace bike
