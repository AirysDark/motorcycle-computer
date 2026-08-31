#pragma once
#include <cstdint>
#include "bike/security_server.hpp"

namespace bike {

class SecurityController {
public:
    SecurityController(BikeNode& node, SecurityHardware& hardware)
        : node_(node), server_(node, hardware) {
        node_.set_auto_ack(false);
    }

    void service(std::uint32_t now_ms);
    SecurityServer& server() { return server_; }
    const SecurityServer& server() const { return server_; }

private:
    bool handle_system_packet(const Packet& packet, std::uint32_t now_ms);
    bool send_device_info(NodeAddress destination, std::uint32_t now_ms);
    bool send_heartbeat(NodeAddress destination, std::uint32_t now_ms);

    BikeNode& node_;
    SecurityServer server_;
};

} // namespace bike
