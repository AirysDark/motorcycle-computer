#include "bike/supervisor.hpp"

namespace bike {
namespace {
std::uint32_t get_u32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}
}

NetworkSupervisor::NetworkSupervisor(BikeNode& node) : node_(node) {}

NodeStatus* NetworkSupervisor::find_or_create(NodeAddress address) {
    for (auto& entry : nodes_) if (entry.discovered && entry.address == address) return &entry;
    for (auto& entry : nodes_) if (!entry.discovered) { entry.address = address; entry.discovered = true; return &entry; }
    return nullptr;
}

const NodeStatus* NetworkSupervisor::status(NodeAddress address) const {
    for (const auto& entry : nodes_) if (entry.discovered && entry.address == address) return &entry;
    return nullptr;
}

std::size_t NetworkSupervisor::known_node_count() const {
    std::size_t count = 0; for (const auto& entry : nodes_) if (entry.discovered) ++count; return count;
}

bool NetworkSupervisor::send_discovery(std::uint32_t now_ms) {
    Packet packet{}; packet.destination = NodeAddress::Broadcast; packet.type = MessageType::DeviceDiscovery; packet.length = 0;
    return node_.send(packet, now_ms, false);
}

bool NetworkSupervisor::send_heartbeat(std::uint32_t now_ms) {
    Packet packet{}; packet.destination = NodeAddress::Broadcast; packet.type = MessageType::Heartbeat; packet.length = 0;
    if (!node_.send(packet, now_ms, false)) return false; last_heartbeat_tx_ms_ = now_ms; return true;
}

void NetworkSupervisor::observe(const Packet& packet, std::uint32_t now_ms) {
    auto* entry = find_or_create(packet.source); if (!entry) return;
    entry->online = true; entry->last_seen_ms = now_ms;

    if (packet.type == MessageType::DeviceInfo && packet.length >= kDeviceInfoPayloadSize && packet.payload[0] == kDeviceInfoSchemaVersion) {
        entry->identity_schema = packet.payload[0];
        entry->address = static_cast<NodeAddress>(packet.payload[1]);
        entry->device_class = static_cast<DeviceClass>(packet.payload[2]);
        entry->protocol_version = packet.payload[3];
        entry->firmware_major = packet.payload[4];
        entry->firmware_minor = packet.payload[5];
        entry->firmware_patch = packet.payload[6];
        entry->hardware_revision = packet.payload[7];
        entry->reset_reason = static_cast<ResetReason>(packet.payload[8]);
        entry->uptime_ms = get_u32(&packet.payload[10]);
        entry->capabilities = static_cast<std::uint16_t>((static_cast<std::uint16_t>(packet.payload[14]) << 8) | packet.payload[15]);
        entry->protocol_compatible = entry->protocol_version == kProtocolVersion;
        entry->identity_valid = true;
    } else if (packet.type == MessageType::Heartbeat) {
        entry->last_heartbeat_ms = now_ms;
    } else if (packet.type == MessageType::Fault) {
        ++entry->fault_count;
    }
}

void NetworkSupervisor::service(std::uint32_t now_ms) {
    if (static_cast<std::uint32_t>(now_ms - last_heartbeat_tx_ms_) >= heartbeat_interval_ms_) send_heartbeat(now_ms);
    for (auto& entry : nodes_) {
        if (!entry.discovered || !entry.online) continue;
        if (static_cast<std::uint32_t>(now_ms - entry.last_seen_ms) >= offline_timeout_ms_) entry.online = false;
    }
}

} // namespace bike
