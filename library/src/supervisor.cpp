#include "bike/supervisor.hpp"

namespace bike {

NetworkSupervisor::NetworkSupervisor(BikeNode& node) : node_(node) {}

NodeStatus* NetworkSupervisor::find_or_create(NodeAddress address) {
    for (auto& entry : nodes_) {
        if (entry.discovered && entry.address == address) return &entry;
    }
    for (auto& entry : nodes_) {
        if (!entry.discovered) {
            entry.address = address;
            entry.discovered = true;
            return &entry;
        }
    }
    return nullptr;
}

const NodeStatus* NetworkSupervisor::status(NodeAddress address) const {
    for (const auto& entry : nodes_) {
        if (entry.discovered && entry.address == address) return &entry;
    }
    return nullptr;
}

std::size_t NetworkSupervisor::known_node_count() const {
    std::size_t count = 0;
    for (const auto& entry : nodes_) if (entry.discovered) ++count;
    return count;
}

bool NetworkSupervisor::send_discovery(std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = NodeAddress::Broadcast;
    packet.type = MessageType::DeviceDiscovery;
    packet.length = 0;
    return node_.send(packet, now_ms, false);
}

bool NetworkSupervisor::send_heartbeat(std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = NodeAddress::Broadcast;
    packet.type = MessageType::Heartbeat;
    packet.length = 0;
    if (!node_.send(packet, now_ms, false)) return false;
    last_heartbeat_tx_ms_ = now_ms;
    return true;
}

void NetworkSupervisor::observe(const Packet& packet, std::uint32_t now_ms) {
    auto* entry = find_or_create(packet.source);
    if (!entry) return;

    entry->online = true;
    entry->last_seen_ms = now_ms;

    if (packet.type == MessageType::Heartbeat) {
        entry->last_heartbeat_ms = now_ms;
    } else if (packet.type == MessageType::Fault) {
        ++entry->fault_count;
    }
}

void NetworkSupervisor::service(std::uint32_t now_ms) {
    if (static_cast<std::uint32_t>(now_ms - last_heartbeat_tx_ms_) >= heartbeat_interval_ms_) {
        send_heartbeat(now_ms);
    }

    for (auto& entry : nodes_) {
        if (!entry.discovered || !entry.online) continue;
        if (static_cast<std::uint32_t>(now_ms - entry.last_seen_ms) >= offline_timeout_ms_) {
            entry.online = false;
        }
    }
}

} // namespace bike
