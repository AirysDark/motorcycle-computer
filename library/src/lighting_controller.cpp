#include "bike/lighting_controller.hpp"

namespace bike {

bool LightingController::send_device_info(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::DeviceInfo;
    packet.flags = FlagResponse;
    packet.length = 3;
    packet.payload[0] = to_u8(NodeAddress::Lighting);
    packet.payload[1] = 0x01; // device class: lighting
    packet.payload[2] = kProtocolVersion;
    return node_.send(packet, now_ms, false);
}

bool LightingController::send_heartbeat(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::Heartbeat;
    packet.flags = FlagResponse;
    packet.length = 0;
    return node_.send(packet, now_ms, false);
}

bool LightingController::handle_system_packet(const Packet& packet, std::uint32_t now_ms) {
    if (packet.type == MessageType::DeviceDiscovery) {
        if ((packet.flags & FlagAckRequired) != 0) {
            node_.send_ack(packet.source, packet.sequence, false);
        }
        send_device_info(packet.source, now_ms);
        return true;
    }

    if (packet.type == MessageType::Heartbeat) {
        if ((packet.flags & FlagAckRequired) != 0) {
            node_.send_ack(packet.source, packet.sequence, false);
        }
        send_heartbeat(packet.source, now_ms);
        return true;
    }

    return false;
}

void LightingController::service(std::uint32_t now_ms) {
    Packet packet{};
    while (node_.poll(packet, now_ms)) {
        if (handle_system_packet(packet, now_ms)) continue;

        if (server_.handle_packet(packet, now_ms)) continue;

        if ((packet.flags & FlagAckRequired) != 0) {
            node_.send_ack(packet.source, packet.sequence, true);
        }
    }

    node_.service(now_ms);
}

} // namespace bike
