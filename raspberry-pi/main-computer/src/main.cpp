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
    return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
const char* online_text(const bike::NodeStatus* status) {
    if (status == nullptr || !status->discovered) return "UNKNOWN";
    return status->online ? "ONLINE" : "OFFLINE";
}
const char* on_off(bool value) { return value ? "ON" : "OFF"; }
const char* security_mode_text(bike::SecurityMode mode) {
    switch (mode) {
        case bike::SecurityMode::Unlocked: return "UNLOCKED";
        case bike::SecurityMode::Locked: return "LOCKED";
        case bike::SecurityMode::Alarm: return "ALARM";
        case bike::SecurityMode::LockPending: return "LOCK_PENDING";
    }
    return "UNKNOWN";
}
const char* electrical_text(bike::LightingElectricalStatus status) {
    switch (status) {
        case bike::LightingElectricalStatus::Unknown: return "UNKNOWN";
        case bike::LightingElectricalStatus::Off: return "OFF";
        case bike::LightingElectricalStatus::Ok: return "OK";
        case bike::LightingElectricalStatus::OpenLoad: return "OPEN_LOAD";
        case bike::LightingElectricalStatus::OverCurrent: return "OVER_CURRENT";
    }
    return "UNKNOWN";
}
const char* output_text(bike::LightingOutput output) {
    switch (output) {
        case bike::LightingOutput::LeftIndicator: return "left";
        case bike::LightingOutput::RightIndicator: return "right";
        case bike::LightingOutput::BrakeBright: return "brake";
        case bike::LightingOutput::HighBeam: return "high";
    }
    return "unknown";
}

void print_status(const bike::MotorcycleSnapshot& snapshot) {
    std::cout << "northbridge=" << online_text(snapshot.northbridge_node)
              << " lighting-node=" << online_text(snapshot.lighting_node)
              << " security-node=" << online_text(snapshot.security_node)
              << " tx_failures=" << snapshot.tx_failures
              << " rx_drops=" << snapshot.rx_drops << '\n';

    if (snapshot.northbridge.valid) {
        std::cout << "northbridge-network: forwarded=" << snapshot.northbridge.forwarded_packets
                  << " dropped=" << snapshot.northbridge.dropped_packets
                  << " route-movements=" << snapshot.northbridge.route_movement_events
                  << " topology-faults=" << snapshot.northbridge.topology_fault_events << '\n';
        for (std::size_t i = 0; i < snapshot.northbridge.ports.size(); ++i) {
            const auto& port = snapshot.northbridge.ports[i];
            if (!port.attached) continue;
            std::cout << "northbridge-port " << i
                      << ": rx_packets=" << port.rx_packets
                      << " tx_packets=" << port.tx_packets
                      << " rx_bytes=" << port.rx_bytes
                      << " dropped=" << port.dropped_packets
                      << " malformed=" << port.malformed_frames << '\n';
        }
        if (snapshot.northbridge.topology_fault_active) {
            std::cout << "northbridge-topology-fault: node=0x" << std::hex
                      << static_cast<unsigned>(bike::to_u8(snapshot.northbridge.topology_fault_node))
                      << std::dec
                      << " expected-port=" << static_cast<unsigned>(snapshot.northbridge.expected_port)
                      << " actual-port=" << static_cast<unsigned>(snapshot.northbridge.actual_port) << '\n';
        }
    } else {
        std::cout << "northbridge-network: diagnostics unknown\n";
    }

    if (snapshot.lighting.valid) {
        std::cout << "lighting: left=" << on_off(snapshot.lighting.left_indicator)
                  << " right=" << on_off(snapshot.lighting.right_indicator)
                  << " brake=" << on_off(snapshot.lighting.brake_bright)
                  << " high=" << on_off(snapshot.lighting.high_beam) << '\n';
    } else {
        std::cout << "lighting: state unknown\n";
    }

    if (snapshot.lighting_diagnostics.valid) {
        for (const auto& channel : snapshot.lighting_diagnostics.channels) {
            std::cout << "lighting-electrical " << output_text(channel.output)
                      << ": driven=" << on_off(channel.driven)
                      << " feedback=" << (channel.feedback_available ? "YES" : "NO")
                      << " current_ma=" << channel.current_ma
                      << " status=" << electrical_text(channel.status) << '\n';
        }
    }

    const bike::LightingOutput outputs[] = {
        bike::LightingOutput::LeftIndicator, bike::LightingOutput::RightIndicator,
        bike::LightingOutput::BrakeBright, bike::LightingOutput::HighBeam
    };
    for (std::size_t i = 0; i < 4; ++i) {
        const auto& fault = snapshot.lighting_faults[i];
        if (!fault.latched && fault.occurrence_count == 0) continue;
        std::cout << "lighting-fault " << output_text(outputs[i])
                  << ": latched=" << (fault.latched ? "YES" : "NO")
                  << " status=" << electrical_text(fault.status)
                  << " occurrences=" << fault.occurrence_count
                  << " shutdown=" << (fault.shutdown_applied ? "YES" : "NO") << '\n';
    }

    if (snapshot.security.valid) {
        std::cout << "security: mode=" << security_mode_text(snapshot.security.mode)
                  << " inhibit=" << on_off(snapshot.security.start_inhibit)
                  << " alarm=" << on_off(snapshot.security.alarm_active)
                  << " shock-warning=" << on_off(snapshot.security.shock_warning)
                  << " shock-trigger=" << on_off(snapshot.security.shock_trigger)
                  << " engine-running=" << on_off(snapshot.security.engine_running) << '\n';
    } else {
        std::cout << "security: state unknown\n";
    }
}

bool handle_command(const std::string& line, bike::MotorcycleComputer& computer, std::uint32_t now_ms) {
    if (line == "quit" || line == "exit") return false;
    if (line == "status") print_status(computer.snapshot());
    else if (line == "left on") computer.lighting().set_left_indicator(true, now_ms);
    else if (line == "left off") computer.lighting().set_left_indicator(false, now_ms);
    else if (line == "right on") computer.lighting().set_right_indicator(true, now_ms);
    else if (line == "right off") computer.lighting().set_right_indicator(false, now_ms);
    else if (line == "brake on") computer.lighting().set_brake_bright(true, now_ms);
    else if (line == "brake off") computer.lighting().set_brake_bright(false, now_ms);
    else if (line == "high on") computer.lighting().set_high_beam(true, now_ms);
    else if (line == "high off") computer.lighting().set_high_beam(false, now_ms);
    else if (line == "lock") computer.security().lock(now_ms);
    else if (line == "unlock") computer.security().unlock(now_ms);
    else if (line == "silence") computer.security().silence_alarm(now_ms);
    else if (line == "refresh") computer.request_all_states(now_ms);
    else if (!line.empty()) std::cout << "Unknown command\n";
    return true;
}
}

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
