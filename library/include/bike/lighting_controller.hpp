#pragma once
#include <cstdint>
#include "bike/lighting_server.hpp"

namespace bike {

class LightingController {
public:
    LightingController(BikeNode& node, LightingHardware& hardware)
        : node_(node), server_(node, hardware) {
        node_.set_auto_ack(false);
    }

    void service(std::uint32_t now_ms);
    LightingServer& server() { return server_; }
    const LightingServer& server() const { return server_; }

private:
    bool handle_system_packet(const Packet& packet, std::uint32_t now_ms);
    bool send_device_info(NodeAddress destination, std::uint32_t now_ms);
    bool send_heartbeat(NodeAddress destination, std::uint32_t now_ms);

    BikeNode& node_;
    LightingServer server_;
};

} // namespace bike
