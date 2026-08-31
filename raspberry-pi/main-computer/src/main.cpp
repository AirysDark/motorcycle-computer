#include <chrono>
#include <cstdint>
#include <iostream>
#include <poll.h>
#include <string>

#include "bike/motorcycle_computer.hpp"
#include "posix_serial_transport.hpp"

namespace {

std::uint32_t monotonic_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

const char* online_text(const bike::NodeStatus* status) {
    if (status == nullptr || !status->discovered) return "UNKNOWN";
    return status->online ? "ONLINE" : "OFFLINE";
}

const char* on_off(bool value) {
    return value ? "ON" : "OFF";
}

const char* security_mode_text(bike::SecurityMode mode) {
    switch (mode) {
        case bike::SecurityMode::Unlocked: return "UNLOCKED";
        case bike::SecurityMode::Locked:   return "LOCKED";
        case bike::SecurityMode::Alarm:    return "ALARM";
    }
    return "UNKNOWN";
}

void print_status(const bike::MotorcycleSnapshot& snapshot) {
    std::cout
        << "northbridge=" << online_text(snapshot.northbridge_node)
        << " lighting-node=" << online_text(snapshot.lighting_node)
        << " security-node=" << online_text(snapshot.security_node)
        << " tx_failures=" << snapshot.tx_failures
        << " rx_drops=" << snapshot.rx_drops
        << '\n';

    if (snapshot.lighting.valid) {
        std::cout
            << "lighting: left=" << on_off(snapshot.lighting.left_indicator)
            << " right=" << on_off(snapshot.lighting.right_indicator)
            << " brake=" << on_off(snapshot.lighting.brake_bright)
            << " high=" << on_off(snapshot.lighting.high_beam)
            << '\n';
    } else {
        std::cout << "lighting: state unknown\n";
    }

    if (snapshot.security.valid) {
        std::cout
            << "security: mode=" << security_mode_text(snapshot.security.mode)
            << " inhibit=" << on_off(snapshot.security.start_inhibit)
            << " alarm=" << on_off(snapshot.security.alarm_active)
            << " shock-warning=" << on_off(snapshot.security.shock_warning)
            << " shock-trigger=" << on_off(snapshot.security.shock_trigger)
            << " engine-running=" << on_off(snapshot.security.engine_running)
            << '\n';
    } else {
        std::cout << "security: state unknown\n";
    }
}

bool handle_command(const std::string& line, bike::MotorcycleComputer& computer, std::uint32_t now_ms) {
    if (line == "quit" || line == "exit") return false;
    if (line == "status") {
        print_status(computer.snapshot());
    } else if (line == "left on") {
        computer.lighting().set_left_indicator(true, now_ms);
    } else if (line == "left off") {
        computer.lighting().set_left_indicator(false, now_ms);
    } else if (line == "right on") {
        computer.lighting().set_right_indicator(true, now_ms);
    } else if (line == "right off") {
        computer.lighting().set_right_indicator(false, now_ms);
    } else if (line == "brake on") {
        computer.lighting().set_brake_bright(true, now_ms);
    } else if (line == "brake off") {
        computer.lighting().set_brake_bright(false, now_ms);
    } else if (line == "high on") {
        computer.lighting().set_high_beam(true, now_ms);
    } else if (line == "high off") {
        computer.lighting().set_high_beam(false, now_ms);
    } else if (line == "lock") {
        computer.security().lock(now_ms);
    } else if (line == "unlock") {
        computer.security().unlock(now_ms);
    } else if (line == "silence") {
        computer.security().silence_alarm(now_ms);
    } else if (line == "refresh") {
        computer.request_all_states(now_ms);
    } else if (!line.empty()) {
        std::cout << "Unknown command\n";
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    const std::string device = argc > 1 ? argv[1] : "/dev/serial0";

    PosixSerialTransport transport;
    if (!transport.open_device(device, 115200)) {
        std::cerr << "Failed to open northbridge serial device: " << device << '\n';
        return 1;
    }

    bike::BikeNode node(bike::NodeAddress::MainComputer, transport);
    bike::MotorcycleComputer computer(node);
    computer.begin(monotonic_ms());

    std::cout << "Motorcycle computer connected on " << device << '\n';
    std::cout << "Commands: status, refresh, left on|off, right on|off, brake on|off, high on|off, lock, unlock, silence, quit\n> " << std::flush;

    pollfd stdin_poll{};
    stdin_poll.fd = 0;
    stdin_poll.events = POLLIN;

    std::string line;
    bool running = true;
    while (running) {
        computer.service(monotonic_ms());

        const int ready = ::poll(&stdin_poll, 1, 10);
        if (ready < 0) break;
        if (ready == 0) continue;

        if ((stdin_poll.revents & POLLIN) != 0) {
            if (!std::getline(std::cin, line)) break;
            running = handle_command(line, computer, monotonic_ms());
            if (running) std::cout << "> " << std::flush;
        }
    }

    return 0;
}
