#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/router.hpp"
#include "bike/stream_parser.hpp"
#include "bike/transport.hpp"

namespace bike {

class NorthbridgeRuntime {
public:
    bool attach_port(std::uint8_t port, Transport& transport);
    bool set_route(NodeAddress address, std::uint8_t port) {
        return router_.set_route(address, port);
    }

    void service(std::uint32_t now_ms);
    std::uint32_t forwarded_packets() const { return forwarded_packets_; }
    std::uint32_t dropped_packets() const { return dropped_packets_; }

    NorthbridgeRouter& router() { return router_; }
    const NorthbridgeRouter& router() const { return router_; }

private:
    void service_port(std::uint8_t port, std::uint32_t now_ms);

    NorthbridgeRouter router_{};
    std::array<Transport*, kMaxRouterPorts> transports_{};
    std::array<StreamParser, kMaxRouterPorts> parsers_{};
    std::uint32_t forwarded_packets_{0};
    std::uint32_t dropped_packets_{0};
};

} // namespace bike
