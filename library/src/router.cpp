#include "bike/router.hpp"

namespace bike {

bool NorthbridgeRouter::attach_port(std::uint8_t port, Transport& transport) {
    if (port >= ports_.size()) return false;
    ports_[port] = &transport;
    return true;
}

bool NorthbridgeRouter::set_route(NodeAddress address, std::uint8_t port) {
    if (port >= ports_.size() || ports_[port] == nullptr) return false;

    for (auto& route : routes_) {
        if (route.active && route.address == address) {
            route.port = port;
            return true;
        }
    }

    for (auto& route : routes_) {
        if (!route.active) {
            route.address = address;
            route.port = port;
            route.active = true;
            route.last_seen_ms = 0;
            return true;
        }
    }

    return false;
}

bool NorthbridgeRouter::remove_route(NodeAddress address) {
    for (auto& route : routes_) {
        if (route.active && route.address == address) {
            route = RouteEntry{};
            return true;
        }
    }
    return false;
}

int NorthbridgeRouter::route_for(NodeAddress address) const {
    for (const auto& route : routes_) {
        if (route.active && route.address == address) {
            return static_cast<int>(route.port);
        }
    }
    return -1;
}

bool NorthbridgeRouter::write_to_port(std::uint8_t port, const Packet& packet) {
    if (port >= ports_.size() || ports_[port] == nullptr) return false;

    std::array<std::uint8_t, kMaxFrameSize> frame{};
    std::size_t length = 0;
    if (!encode_packet(packet, frame.data(), frame.size(), length)) return false;
    return ports_[port]->write(frame.data(), length);
}

bool NorthbridgeRouter::forward(const Packet& packet, std::uint8_t ingress_port, std::uint32_t now_ms) {
    mark_seen(packet.source, ingress_port, now_ms);

    if (packet.destination == NodeAddress::Northbridge) {
        return true;
    }

    if (packet.destination == NodeAddress::Broadcast) {
        bool forwarded = false;
        for (std::uint8_t port = 0; port < ports_.size(); ++port) {
            if (port == ingress_port || ports_[port] == nullptr) continue;
            forwarded = write_to_port(port, packet) || forwarded;
        }
        return forwarded;
    }

    const int destination_port = route_for(packet.destination);
    if (destination_port < 0 || static_cast<std::uint8_t>(destination_port) == ingress_port) {
        return false;
    }

    return write_to_port(static_cast<std::uint8_t>(destination_port), packet);
}

void NorthbridgeRouter::mark_seen(NodeAddress address, std::uint8_t ingress_port, std::uint32_t now_ms) {
    for (auto& route : routes_) {
        if (route.active && route.address == address) {
            route.port = ingress_port;
            route.last_seen_ms = now_ms;
            return;
        }
    }

    set_route(address, ingress_port);
    for (auto& route : routes_) {
        if (route.active && route.address == address) {
            route.last_seen_ms = now_ms;
            return;
        }
    }
}

bool NorthbridgeRouter::is_online(NodeAddress address, std::uint32_t now_ms, std::uint32_t timeout_ms) const {
    for (const auto& route : routes_) {
        if (route.active && route.address == address) {
            return static_cast<std::uint32_t>(now_ms - route.last_seen_ms) <= timeout_ms;
        }
    }
    return false;
}

} // namespace bike
