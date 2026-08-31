#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

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

void print_status(const bike::MotorcycleSnapshot& snapshot) {
    std::cout
        << "northbridge=" << online_text(snapshot.northbridge)
        << " lighting=" << online_text(snapshot.lighting)
        << " security=" << online_text(snapshot.security)
        << " tx_failures=" << snapshot.tx_failures
        << '\n';
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

    const auto start_ms = monotonic_ms();
    computer.begin(start_ms);

    std::cout << "Motorcycle computer connected on " << device << '\n';
    std::cout << "Commands: status, left on|off, right on|off, brake on|off, high on|off, lock, unlock, silence, quit\n";

    std::string line;
    while (true) {
        const auto now_ms = monotonic_ms();
        computer.service(now_ms);

        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        if (line == "quit" || line == "exit") break;
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
        } else {
            std::cout << "Unknown command\n";
        }

        // Give the non-blocking protocol loop a short service window after a command.
        for (int i = 0; i < 10; ++i) {
            computer.service(monotonic_ms());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    return 0;
}
