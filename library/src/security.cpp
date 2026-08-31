#include "bike/security.hpp"

namespace bike {

bool SecurityClient::command(SecurityCommand command, std::uint32_t now_ms, bool require_ack) {
    Packet packet{};
    packet.destination = NodeAddress::Security;
    packet.type = MessageType::SecurityCommand;
    packet.length = 1;
    packet.payload[0] = static_cast<std::uint8_t>(command);
    return node_.send(packet, now_ms, require_ack);
}

} // namespace bike
