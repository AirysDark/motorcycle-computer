# Security Southbridge

ESP32-WROOM firmware for the local motorcycle security controller.

## Implemented scope

- Lock / arm
- Unlock / disarm
- Shock warning input
- Shock trigger input
- Alarm / siren output
- Start-inhibit output
- Engine-running sense input
- Security state and event reporting
- Discovery and heartbeat responses
- ACK/NACK after application validation

## Safety rule

The start-inhibit is a **start prevention** function, not a running-engine kill command. The firmware will not assert the logical inhibit while the engine-running input is active. If the bike is locked while already running, the inhibit is deferred until the engine-running input goes inactive.

The final relay/driver wiring must preserve this property electrically as well as in software.

## Current bench pins

| Function | GPIO |
|---|---:|
| Northbridge RX | 16 |
| Northbridge TX | 17 |
| Shock warning | 25 |
| Shock trigger | 26 |
| Engine running sense | 27 |
| Alarm output | 32 |
| Start inhibit | 33 |

These are development defaults only and must be checked against the final PCB/harness.

## Fob support

Fob authentication is intentionally not implemented yet. Protocol IDs are reserved for `AUTH_REQUEST`, `AUTH_RESULT` and `FOB_PRESENT` so a future fob receiver can feed the same local security state machine without redesigning lock/unlock behavior.

## Reset behavior

The current development firmware starts with the logical security state unlocked and both outputs inactive. Persistent armed state is a future feature and must only be restored in a way that cannot unexpectedly inhibit a running engine.
