#include "bike/node.hpp"

namespace bike {

BikeNode::BikeNode(NodeAddress address, Transport& transport)
    : address_(address), transport_(transport) {}

std::uint16_t BikeNode::next_sequence() {
    const auto value = sequence_++;
    if (sequence_ == 0) sequence_ = 1;
    return value;
}

bool BikeNode::write_packet(const Packet& packet) {
    std::array<std::uint8_t, kMaxFrameSize> frame{};
    std::size_t length = 0;
    if (!encode_packet(packet, frame.data(), frame.size(), length)) return false;
    return transport_.write(frame.data(), length);
}

bool BikeNode::send(Packet packet, std::uint32_t now_ms, bool require_ack) {
    packet.source = address_;
    if (packet.sequence == 0) packet.sequence = next_sequence();

    if (require_ack) {
        if (pending_.active) return false;
        packet.flags |= FlagAckRequired;
    }

    if (!write_packet(packet)) return false;

    if (require_ack) {
        pending_.active = true;
        pending_.packet = packet;
        pending_.sent_at_ms = now_ms;
        pending_.retries = 0;
    }

    return true;
}

bool BikeNode::send_ack(NodeAddress destination, std::uint16_t sequence, bool nack) {
    Packet packet{};
    packet.source = address_;
    packet.destination = destination;
    packet.type = nack ? MessageType::Nack : MessageType::Ack;
    packet.flags = FlagResponse;
    packet.sequence = sequence;
    packet.length = 0;
    return write_packet(packet);
}

void BikeNode::handle_control_packet(const Packet& packet) {
    if (!pending_.active) return;
    if (packet.sequence != pending_.packet.sequence) return;
    if (packet.source != pending_.packet.destination) return;

    if (packet.type == MessageType::Ack) {
        pending_.active = false;
    } else if (packet.type == MessageType::Nack) {
        pending_.sent_at_ms = 0;
    }
}

bool BikeNode::poll(Packet& out_packet, std::uint32_t now_ms) {
    (void)now_ms;
    std::array<std::uint8_t, 64> chunk{};
    const auto count = transport_.read(chunk.data(), chunk.size());

    for (std::size_t i = 0; i < count; ++i) {
        Packet packet{};
        if (!parser_.push(chunk[i], packet)) continue;

        if (packet.destination != address_ && packet.destination != NodeAddress::Broadcast) {
            continue;
        }

        if (packet.type == MessageType::Ack || packet.type == MessageType::Nack) {
            handle_control_packet(packet);
            continue;
        }

        if ((packet.flags & FlagAckRequired) != 0) {
            send_ack(packet.source, packet.sequence, false);
        }

        out_packet = packet;
        return true;
    }

    return false;
}

void BikeNode::service(std::uint32_t now_ms) {
    if (!pending_.active) return;
    if (static_cast<std::uint32_t>(now_ms - pending_.sent_at_ms) < ack_timeout_ms_) return;

    if (pending_.retries >= max_retries_) {
        pending_.active = false;
        ++tx_failures_;
        return;
    }

    if (write_packet(pending_.packet)) {
        ++pending_.retries;
        pending_.sent_at_ms = now_ms;
    }
}

} // namespace bike
