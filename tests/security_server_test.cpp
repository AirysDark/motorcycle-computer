#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "bike/security_controller.hpp"
#include "bike/transport.hpp"

namespace {

class LoopbackTransport final : public bike::Transport {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        written.insert(written.end(), data, data + length);
        return true;
    }
    std::size_t read(std::uint8_t*, std::size_t) override { return 0; }
    std::vector<std::uint8_t> written;
};

class FakeSecurityHardware final : public bike::SecurityHardware {
public:
    bool set_alarm_output(bool enabled) override { alarm = enabled; return true; }
    bool set_start_inhibit(bool enabled) override { inhibit = enabled; return true; }
    bool shock_warning_active() const override { return warning; }
    bool shock_trigger_active() const override { return trigger; }
    bool engine_running() const override { return running; }
    bool alarm_output_active() const override { return alarm; }
    bool start_inhibit_active() const override { return inhibit; }

    bool warning{false};
    bool trigger{false};
    bool running{false};
    bool alarm{false};
    bool inhibit{false};
};

} // namespace

int main() {
    LoopbackTransport transport;
    bike::BikeNode node(bike::NodeAddress::Security, transport);
    FakeSecurityHardware hardware;
    bike::SecurityServer server(node, hardware);

    bike::Packet lock{};
    lock.source = bike::NodeAddress::MainComputer;
    lock.destination = bike::NodeAddress::Security;
    lock.type = bike::MessageType::SecurityCommand;
    lock.flags = bike::FlagAckRequired;
    lock.sequence = 1;
    lock.length = 1;
    lock.payload[0] = static_cast<std::uint8_t>(bike::SecurityCommand::Lock);

    assert(server.handle_packet(lock, 100));
    assert(server.mode() == bike::SecurityMode::Locked);
    assert(hardware.inhibit);

    hardware.trigger = true;
    server.service(200);
    assert(server.mode() == bike::SecurityMode::Alarm);
    assert(hardware.alarm);

    bike::Packet unlock = lock;
    unlock.sequence = 2;
    unlock.payload[0] = static_cast<std::uint8_t>(bike::SecurityCommand::Unlock);
    assert(server.handle_packet(unlock, 300));
    assert(server.mode() == bike::SecurityMode::Unlocked);
    assert(!hardware.inhibit);
    assert(!hardware.alarm);

    // Safety behavior: locking while running must never assert start inhibit.
    hardware.running = true;
    bike::Packet relock = lock;
    relock.sequence = 3;
    assert(server.handle_packet(relock, 400));
    assert(server.mode() == bike::SecurityMode::Locked);
    assert(!hardware.inhibit);

    // Once the engine is no longer running, the inhibit may safely engage.
    hardware.running = false;
    server.service(500);
    assert(hardware.inhibit);

    return 0;
}
