#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>
#include "bike/lighting.hpp"
#include "bike/supervisor.hpp"
#include "bike_protocol/codec.hpp"

class LoopbackTransport : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        writes.emplace_back(data, data + length);
        for (std::size_t i = 0; i < length; ++i) rx.push_back(data[i]);
        return true;
    }

    std::size_t read(std::uint8_t* data, std::size_t capacity) override {
        std::size_t count = 0;
        while (count < capacity && !rx.empty()) {
            data[count++] = rx.front();
            rx.pop_front();
        }
        return count;
    }

    std::deque<std::uint8_t> rx;
    std::vector<std::vector<std::uint8_t>> writes;
};

int main() {
    LoopbackTransport transport;
    bike::BikeNode node(bike::NodeAddress::MainComputer, transport);
    bike::LightingClient lighting(node);

    assert(lighting.set_left_indicator(true, 100, false));
    assert(!transport.writes.empty());

    bike::Packet decoded{};
    assert(bike::decode_packet(transport.writes.back().data(), transport.writes.back().size(), decoded) == bike::DecodeStatus::Ok);
    assert(decoded.destination == bike::NodeAddress::Lighting);
    assert(decoded.type == bike::MessageType::SetOutput);
    assert(decoded.length == 2);
    assert(decoded.payload[0] == static_cast<std::uint8_t>(bike::LightingOutput::LeftIndicator));
    assert(decoded.payload[1] == 1);

    bike::NetworkSupervisor supervisor(node);
    supervisor.observe(decoded, 500);
    auto* status = supervisor.status(bike::NodeAddress::MainComputer);
    assert(status != nullptr);
    assert(status->online);

    bike::Packet heartbeat{};
    heartbeat.source = bike::NodeAddress::Lighting;
    heartbeat.destination = bike::NodeAddress::MainComputer;
    heartbeat.type = bike::MessageType::Heartbeat;
    supervisor.observe(heartbeat, 1000);
    status = supervisor.status(bike::NodeAddress::Lighting);
    assert(status != nullptr);
    assert(status->online);
    assert(status->last_heartbeat_ms == 1000);

    supervisor.set_offline_timeout_ms(2000);
    supervisor.service(3100);
    status = supervisor.status(bike::NodeAddress::Lighting);
    assert(status != nullptr);
    assert(!status->online);

    return 0;
}
