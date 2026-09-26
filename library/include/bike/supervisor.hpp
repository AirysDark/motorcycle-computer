#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/node.hpp"
#include "bike_protocol/device_info.hpp"

namespace bike {
enum class NodeHealth : std::uint8_t { Unknown=0, Online=1, Degraded=2, Offline=3, Incompatible=4 };
struct NodeStatus {
    NodeAddress address{NodeAddress::Broadcast}; bool discovered{false}; bool online{false}; NodeHealth health{NodeHealth::Unknown}; bool required{false};
    std::uint32_t last_seen_ms{0}; std::uint32_t last_heartbeat_ms{0}; std::uint32_t fault_count{0}; std::uint32_t reboot_count{0};
    bool identity_valid{false}; std::uint8_t identity_schema{0}; DeviceClass device_class{DeviceClass::MainComputer}; std::uint8_t protocol_version{0};
    std::uint8_t firmware_major{0}, firmware_minor{0}, firmware_patch{0}, hardware_revision{0}; ResetReason reset_reason{ResetReason::Unknown};
    std::uint32_t uptime_ms{0}; std::uint16_t capabilities{0}; bool protocol_compatible{false};
};
class NetworkSupervisor {
public:
    explicit NetworkSupervisor(BikeNode& node);
    void set_heartbeat_interval_ms(std::uint32_t v){heartbeat_interval_ms_=v;} void set_offline_timeout_ms(std::uint32_t v){offline_timeout_ms_=v;}
    void set_rediscovery_interval_ms(std::uint32_t v){rediscovery_interval_ms_=v;} bool set_required(NodeAddress address,bool required=true);
    bool send_discovery(std::uint32_t now_ms); bool send_heartbeat(std::uint32_t now_ms); void observe(const Packet& packet,std::uint32_t now_ms); void service(std::uint32_t now_ms);
    const NodeStatus* status(NodeAddress address) const; std::size_t known_node_count() const; const std::array<NodeStatus,16>& nodes() const{return nodes_;}
    bool required_node_fault() const { return required_node_fault_; }
private:
    NodeStatus* find_or_create(NodeAddress address); void update_health(NodeStatus& entry);
    BikeNode& node_; std::array<NodeStatus,16> nodes_{}; std::uint32_t heartbeat_interval_ms_{1000}, offline_timeout_ms_{3000}, rediscovery_interval_ms_{5000};
    std::uint32_t last_heartbeat_tx_ms_{0}, last_discovery_tx_ms_{0}; bool required_node_fault_{false};
};
} // namespace bike
