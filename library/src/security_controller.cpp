#include "bike/security_controller.hpp"

namespace bike {

bool SecurityController::send_device_info(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::DeviceInfo;
    packet.flags = FlagResponse;
    packet.length = 3;
    packet.payload[0] = static_cast<std::uint8_t>(NodeAddress::Security);
    packet.payload[1] = 0x01; // device class: security
    packet.payload[2] = 0x01; // implementation version
    return node_.send(packet, now_ms, false);
}

bool SecurityController::send_heartbeat(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::Heartbeat;
    packet.flags = FlagResponse;
    packet.length = 0;
    return node_.send(packet, now_ms, false);
}

bool SecurityController::handle_system_packet(const Packet& packet, std::uint32_t now_ms) {
    if (packet.type == MessageType::DeviceDiscovery) {
        return send_device_info(packet.source, now_ms);
    }
    if (packet.type == MessageType::Heartbeat) {
        return send_heartbeat(packet.source, now_ms);
    }
    if (packet.type == MessageType::GetState) {
        return server_.publish_state(packet.source, now_ms);
    }
    return false;
}

void SecurityController::service(std::uint32_t now_ms) {
    Packet packet{};
    while (node_.poll(packet, now_ms)) {
        if (handle_system_packet(packet, now_ms)) continue;
        server_.handle_packet(packet, now_ms);
    }

    server_.service(now_ms);
    node_.service(now_ms);
}

} // namespace bike
