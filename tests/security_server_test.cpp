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
    server.set_alarm_duration_ms(1000);

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

    // Trigger must persist for 100 ms before it can start the alarm.
    hardware.trigger = true;
    server.service(200);
    assert(server.mode() == bike::SecurityMode::Locked);
    server.service(299);
    assert(server.mode() == bike::SecurityMode::Locked);
    server.service(300);
    assert(server.mode() == bike::SecurityMode::Alarm);
    assert(hardware.alarm);

    // Alarm duration is bounded. A stuck trigger cannot immediately retrigger it.
    server.service(1299);
    assert(hardware.alarm);
    server.service(1300);
    assert(server.mode() == bike::SecurityMode::Locked);
    assert(!hardware.alarm);
    server.service(1400);
    assert(!hardware.alarm);

    // A genuine release resets the trigger edge detector.
    hardware.trigger = false;
    server.service(1401);

    bike::Packet unlock = lock;
    unlock.sequence = 2;
    unlock.payload[0] = static_cast<std::uint8_t>(bike::SecurityCommand::Unlock);
    assert(server.handle_packet(unlock, 1500));
    assert(server.mode() == bike::SecurityMode::Unlocked);
    assert(!hardware.inhibit);
    assert(!hardware.alarm);

    // Locking while running enters pending immediately and keeps inhibit OFF.
    hardware.running = true;
    bike::Packet relock = lock;
    relock.sequence = 3;
    assert(server.handle_packet(relock, 1600));
    assert(server.mode() == bike::SecurityMode::LockPending);
    assert(!hardware.inhibit);

    // Warning and trigger require persistence, and neither can alarm while pending.
    hardware.warning = true;
    hardware.trigger = true;
    server.service(1700);
    assert(!server.state().shock_warning);
    assert(!server.state().shock_trigger);
    server.service(1749);
    assert(!server.state().shock_warning);
    server.service(1750);
    assert(server.state().shock_warning);
    server.service(1800);
    assert(server.state().shock_trigger);
    assert(server.mode() == bike::SecurityMode::LockPending);
    assert(!hardware.alarm);

    // Engine-running asserts immediately, but engine-stop must remain stable for 250 ms.
    hardware.running = false;
    server.service(1900);
    assert(server.mode() == bike::SecurityMode::LockPending);
    assert(!hardware.inhibit);
    server.service(2149);
    assert(server.mode() == bike::SecurityMode::LockPending);
    server.service(2150);
    assert(server.mode() == bike::SecurityMode::Locked);
    assert(hardware.inhibit);
    assert(!hardware.alarm);

    // A trigger that was already active during pending does not alarm at the arm transition.
    hardware.warning = false;
    hardware.trigger = false;
    server.service(2160);
    assert(!server.state().shock_trigger);

    // After a real release, a new persistent trigger can alarm normally.
    hardware.trigger = true;
    server.service(2200);
    assert(server.mode() == bike::SecurityMode::Locked);
    server.service(2300);
    assert(server.mode() == bike::SecurityMode::Alarm);
    assert(hardware.alarm);

    return 0;
}
