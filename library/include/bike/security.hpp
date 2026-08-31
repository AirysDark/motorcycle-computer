#pragma once
#include <cstdint>
#include "bike/node.hpp"

namespace bike {

enum class SecurityCommand : std::uint8_t {
    Lock          = 0x01,
    Unlock        = 0x02,
    SilenceAlarm  = 0x03,
    RequestState  = 0x04
};

enum class SecurityMode : std::uint8_t {
    Unlocked    = 0x00,
    Locked      = 0x01,
    Alarm       = 0x02,
    LockPending = 0x03
};

enum class SecurityEventCode : std::uint8_t {
    ShockWarning = 0x01,
    ShockTrigger = 0x02,
    AlarmStarted = 0x03,
    AlarmStopped = 0x04,
    Locked       = 0x05,
    Unlocked     = 0x06,
    LockPending  = 0x07
};

class SecurityClient {
public:
    explicit SecurityClient(BikeNode& node) : node_(node) {}

    bool command(SecurityCommand command, std::uint32_t now_ms, bool require_ack = true);
    bool lock(std::uint32_t now_ms, bool require_ack = true) {
        return command(SecurityCommand::Lock, now_ms, require_ack);
    }
    bool unlock(std::uint32_t now_ms, bool require_ack = true) {
        return command(SecurityCommand::Unlock, now_ms, require_ack);
    }
    bool silence_alarm(std::uint32_t now_ms, bool require_ack = true) {
        return command(SecurityCommand::SilenceAlarm, now_ms, require_ack);
    }
    bool request_state(std::uint32_t now_ms) {
        return command(SecurityCommand::RequestState, now_ms, false);
    }

private:
    BikeNode& node_;
};

} // namespace bike
