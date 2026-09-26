#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/node.hpp"
#include "bike_protocol/device_info.hpp"

namespace bike {

struct NodeStatus {
    NodeAddress address{NodeAddress::Broadcast};
    bool discovered{false};
    bool online{false};
    std::uint32_t last_seen_ms{0};
    std::uint32_t last_heartbeat_ms{0};
    std::uint32_t fault_count{0};

    bool identity_valid{false};
    std::uint8_t identity_schema{0};
    DeviceClass device_class{DeviceClass::MainComputer};
    std::uint8_t protocol_version{0};
    std::uint8_t firmware_major{0};
    std::uint8_t firmware_minor{0};
    std::uint8_t firmware_patch{0};
    std::uint8_t hardware_revision{0};
    ResetReason reset_reason{ResetReason::Unknown};
    std::uint32_t uptime_ms{0};
    std::uint16_t capabilities{0};
    bool protocol_compatible{false};
};

class NetworkSupervisor {
public:
    explicit NetworkSupervisor(BikeNode& node);
    void set_heartbeat_interval_ms(std::uint32_t value) { heartbeat_interval_ms_ = value; }
    void set_offline_timeout_ms(std::uint32_t value) { offline_timeout_ms_ = value; }
    bool send_discovery(std::uint32_t now_ms);
    bool send_heartbeat(std::uint32_t now_ms);
    void observe(const Packet& packet, std::uint32_t now_ms);
    void service(std::uint32_t now_ms);
    const NodeStatus* status(NodeAddress address) const;
    std::size_t known_node_count() const;
    const std::array<NodeStatus, 16>& nodes() const { return nodes_; }

private:
    NodeStatus* find_or_create(NodeAddress address);
    BikeNode& node_;
    std::array<NodeStatus, 16> nodes_{};
    std::uint32_t heartbeat_interval_ms_{1000};
    std::uint32_t offline_timeout_ms_{3000};
    std::uint32_t last_heartbeat_tx_ms_{0};
};

} // namespace bike
