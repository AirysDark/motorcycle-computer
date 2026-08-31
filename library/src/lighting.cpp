#include "bike/lighting.hpp"

namespace bike {

bool LightingClient::set_output(LightingOutput output, bool enabled, std::uint32_t now_ms, bool require_ack) {
    Packet packet{};
    packet.destination = NodeAddress::Lighting;
    packet.type = MessageType::SetOutput;
    packet.length = 2;
    packet.payload[0] = static_cast<std::uint8_t>(output);
    packet.payload[1] = enabled ? 1u : 0u;
    return node_.send(packet, now_ms, require_ack);
}

} // namespace bike
