#include "bike/northbridge.hpp"
#include "bike_protocol/codec.hpp"

namespace bike {

namespace {
const NorthbridgePortStats kEmptyStats{};

void put_u32(Packet& packet, std::size_t offset, std::uint32_t value) {
    packet.payload[offset] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    packet.payload[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    packet.payload[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    packet.payload[offset + 3] = static_cast<std::uint8_t>(value & 0xFFu);
}
}

bool NorthbridgeRuntime::attach_port(std::uint8_t port, Transport& transport) {
    if (port >= transports_.size()) return false;
    transports_[port] = &transport;
    return router_.attach_port(port, transport);
}

bool NorthbridgeRuntime::bind_node(NodeAddress address, std::uint8_t port) {
    if (port >= transports_.size() || transports_[port] == nullptr) return false;
    for (auto& binding : bindings_) {
        if (binding.active && binding.address == address) {
            binding.port = port;
            return router_.set_route(address, port);
        }
    }
    for (auto& binding : bindings_) {
        if (!binding.active) {
            binding.address = address;
            binding.port = port;
            binding.active = true;
            return router_.set_route(address, port);
        }
    }
    return false;
}

int NorthbridgeRuntime::expected_port_for(NodeAddress address) const {
    for (const auto& binding : bindings_) {
        if (binding.active && binding.address == address) return static_cast<int>(binding.port);
    }
    return -1;
}

const NorthbridgePortStats& NorthbridgeRuntime::port_stats(std::uint8_t port) const {
    return port < port_stats_.size() ? port_stats_[port] : kEmptyStats;
}

bool NorthbridgeRuntime::send_local(Packet packet, std::uint8_t egress_port) {
    if (egress_port >= transports_.size() || transports_[egress_port] == nullptr) return false;
    packet.source = NodeAddress::Northbridge;
    if (packet.sequence == 0) {
        packet.sequence = local_sequence_++;
        if (local_sequence_ == 0) local_sequence_ = 1;
    }
    std::array<std::uint8_t, kMaxFrameSize> frame{};
    std::size_t length = 0;
    if (!encode_packet(packet, frame.data(), frame.size(), length)) return false;
    const bool ok = transports_[egress_port]->write(frame.data(), length);
    if (ok) ++port_stats_[egress_port].tx_packets;
    return ok;
}

bool NorthbridgeRuntime::send_device_info(NodeAddress destination, std::uint8_t egress_port) {
    Packet response{};
    response.destination = destination;
    response.type = MessageType::DeviceInfo;
    response.flags = FlagResponse;
    response.length = 3;
    response.payload[0] = to_u8(NodeAddress::Northbridge);
    response.payload[1] = 0x03;
    response.payload[2] = kProtocolVersion;
    return send_local(response, egress_port);
}

bool NorthbridgeRuntime::send_heartbeat(NodeAddress destination, std::uint8_t egress_port) {
    Packet response{};
    response.destination = destination;
    response.type = MessageType::Heartbeat;
    response.flags = FlagResponse;
    return send_local(response, egress_port);
}

bool NorthbridgeRuntime::send_topology_fault(NodeAddress address, std::uint8_t expected_port, std::uint8_t actual_port) {
    const int pi_port = router_.route_for(NodeAddress::MainComputer);
    if (pi_port < 0) return false;
    Packet fault{};
    fault.destination = NodeAddress::MainComputer;
    fault.type = MessageType::Fault;
    fault.flags = FlagFault;
    fault.length = 9;
    fault.payload[0] = 0x02; // northbridge subsystem
    fault.payload[1] = 0x01; // topology mismatch
    fault.payload[2] = to_u8(address);
    fault.payload[3] = expected_port;
    fault.payload[4] = actual_port;
    put_u32(fault, 5, topology_fault_events_);
    return send_local(fault, static_cast<std::uint8_t>(pi_port));
}

bool NorthbridgeRuntime::send_diagnostics(NodeAddress destination, std::uint8_t egress_port) {
    Packet response{};
    response.destination = destination;
    response.type = MessageType::Diagnostic;
    response.flags = FlagResponse;
    response.payload[0] = 0x02; // northbridge diagnostics schema v2
    std::uint8_t attached = 0;
    for (const auto* transport : transports_) if (transport != nullptr) ++attached;
    response.payload[1] = attached;
    put_u32(response, 2, forwarded_packets_);
    put_u32(response, 6, dropped_packets_);
    put_u32(response, 10, route_movement_events_);
    put_u32(response, 14, topology_fault_events_);

    std::size_t offset = 18;
    for (std::uint8_t port = 0; port < transports_.size(); ++port) {
        if (transports_[port] == nullptr) continue;
        response.payload[offset] = port;
        put_u32(response, offset + 1, port_stats_[port].rx_packets);
        put_u32(response, offset + 5, port_stats_[port].tx_packets);
        put_u32(response, offset + 9, port_stats_[port].rx_bytes);
        put_u32(response, offset + 13, port_stats_[port].dropped_packets);
        put_u32(response, offset + 17, port_stats_[port].malformed_frames);
        offset += 21;
    }
    response.length = static_cast<std::uint16_t>(offset);
    return send_local(response, egress_port);
}

bool NorthbridgeRuntime::handle_local_packet(const Packet& packet, std::uint8_t ingress_port, std::uint32_t now_ms) {
    (void)now_ms;
    if ((packet.flags & FlagAckRequired) != 0) {
        Packet ack{};
        ack.destination = packet.source;
        ack.type = MessageType::Ack;
        ack.flags = FlagResponse;
        ack.sequence = packet.sequence;
        send_local(ack, ingress_port);
    }
    switch (packet.type) {
        case MessageType::DeviceDiscovery: return send_device_info(packet.source, ingress_port);
        case MessageType::Heartbeat: return send_heartbeat(packet.source, ingress_port);
        case MessageType::Diagnostic:
        case MessageType::GetState: return send_diagnostics(packet.source, ingress_port);
        default: return true;
    }
}

void NorthbridgeRuntime::account_forward(const Packet& packet, std::uint8_t ingress_port, bool success) {
    if (!success) {
        ++port_stats_[ingress_port].dropped_packets;
        return;
    }
    if (packet.destination == NodeAddress::Broadcast) {
        for (std::uint8_t port = 0; port < transports_.size(); ++port) {
            if (port != ingress_port && transports_[port] != nullptr) ++port_stats_[port].tx_packets;
        }
        return;
    }
    const int destination_port = router_.route_for(packet.destination);
    if (destination_port >= 0 && static_cast<std::uint8_t>(destination_port) != ingress_port) {
        ++port_stats_[static_cast<std::uint8_t>(destination_port)].tx_packets;
    }
}

void NorthbridgeRuntime::service_port(std::uint8_t port, std::uint32_t now_ms) {
    if (port >= transports_.size() || transports_[port] == nullptr) return;
    std::array<std::uint8_t, 64> chunk{};
    const auto count = transports_[port]->read(chunk.data(), chunk.size());
    port_stats_[port].rx_bytes += static_cast<std::uint32_t>(count);

    for (std::size_t i = 0; i < count; ++i) {
        Packet packet{};
        if (!parsers_[port].push(chunk[i], packet)) continue;
        ++port_stats_[port].rx_packets;

        const int expected_port = expected_port_for(packet.source);
        if (expected_port >= 0 && static_cast<std::uint8_t>(expected_port) != port) {
            ++route_movement_events_;
            if (topology_enforced_) {
                ++topology_fault_events_;
                ++dropped_packets_;
                ++port_stats_[port].dropped_packets;
                send_topology_fault(packet.source, static_cast<std::uint8_t>(expected_port), port);
                continue;
            }
        }

        if (packet.destination == NodeAddress::Northbridge) {
            router_.mark_seen(packet.source, port, now_ms);
            handle_local_packet(packet, port, now_ms);
            continue;
        }

        const bool forwarded = router_.forward(packet, port, now_ms);
        account_forward(packet, port, forwarded);
        if (forwarded) ++forwarded_packets_;
        else ++dropped_packets_;
    }

    const auto rejected = parsers_[port].rejected_frames();
    if (rejected > parser_reject_snapshot_[port]) {
        port_stats_[port].malformed_frames += rejected - parser_reject_snapshot_[port];
        parser_reject_snapshot_[port] = rejected;
    }
}

void NorthbridgeRuntime::service(std::uint32_t now_ms) {
    for (std::uint8_t port = 0; port < transports_.size(); ++port) service_port(port, now_ms);
}

} // namespace bike
