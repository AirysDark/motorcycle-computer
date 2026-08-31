#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "bike/stream_parser.hpp"
#include "bike/transport.hpp"
#include "bike_protocol/codec.hpp"

namespace bike {

struct PendingTx {
    bool active{false};
    Packet packet{};
    std::uint32_t sent_at_ms{0};
    std::uint8_t retries{0};
};

class BikeNode {
public:
    BikeNode(NodeAddress address, Transport& transport);

    NodeAddress address() const { return address_; }
    std::uint16_t next_sequence();

    bool send(Packet packet, std::uint32_t now_ms, bool require_ack = false);
    bool send_ack(NodeAddress destination, std::uint16_t sequence, bool nack = false);
    bool poll(Packet& out_packet, std::uint32_t now_ms);
    void service(std::uint32_t now_ms);

    void set_ack_timeout_ms(std::uint32_t value) { ack_timeout_ms_ = value; }
    void set_max_retries(std::uint8_t value) { max_retries_ = value; }
    void set_auto_ack(bool enabled) { auto_ack_ = enabled; }
    bool auto_ack() const { return auto_ack_; }
    std::uint32_t tx_failures() const { return tx_failures_; }

private:
    bool write_packet(const Packet& packet);
    void handle_control_packet(const Packet& packet);

    NodeAddress address_;
    Transport& transport_;
    StreamParser parser_{};
    std::uint16_t sequence_{1};
    PendingTx pending_{};
    std::uint32_t ack_timeout_ms_{250};
    std::uint8_t max_retries_{3};
    std::uint32_t tx_failures_{0};
    bool auto_ack_{true};
};

} // namespace bike
