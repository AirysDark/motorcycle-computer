#include "bike/lighting_controller.hpp"
#include "bike_protocol/device_info.hpp"

namespace bike {

bool LightingController::send_device_info(NodeAddress destination, std::uint32_t now_ms) {
    Packet packet{};
    packet.destination = destination;
    packet.type = MessageType::DeviceInfo;
    packet.flags = FlagResponse;
    DeviceIdentity identity{};
    identity.device_class = DeviceClass::LightingController;
    identity.firmware_major = 0;
    identity.firmware_minor = 1;
    identity.firmware_patch = 0;
    identity.hardware_revision = 1;
    identity.capabilities = CapabilityDiagnostics | CapabilityLighting;
    encode_device_info_payload(packet, NodeAddress::Lighting, identity, now_ms);
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
        if ((packet.flags & FlagAckRequired) != 0) node_.send_ack(packet.source, packet.sequence, false);
        send_device_info(packet.source, now_ms);
        return true;
    }
    if (packet.type == MessageType::Heartbeat) {
        if ((packet.flags & FlagAckRequired) != 0) node_.send_ack(packet.source, packet.sequence, false);
        send_heartbeat(packet.source, now_ms);
        return true;
    }
    if (packet.type == MessageType::GetState) {
        if ((packet.flags & FlagAckRequired) != 0) node_.send_ack(packet.source, packet.sequence, false);
        server_.publish_state(packet.source, now_ms);
        return true;
    }
    return false;
}

void LightingController::service(std::uint32_t now_ms) {
    Packet packet{};
    while (node_.poll(packet, now_ms)) {
        if (handle_system_packet(packet, now_ms)) continue;
        if (server_.handle_packet(packet, now_ms)) continue;
        if ((packet.flags & FlagAckRequired) != 0) node_.send_ack(packet.source, packet.sequence, true);
    }
    node_.service(now_ms);
}

} // namespace bike
