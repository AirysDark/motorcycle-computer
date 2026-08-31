# Security Southbridge

ESP32-WROOM firmware for the local motorcycle security controller.

## Implemented scope

- Lock / arm
- Deferred `LOCK_PENDING` state while the engine is running
- Unlock / disarm
- Filtered shock warning input
- Filtered shock trigger input
- Bounded alarm / siren output
- Start-inhibit output
- Filtered engine-running sense input
- Persistent armed intent using ESP32 NVS
- Restart-safe armed-state recovery
- Security state and event reporting
- Discovery and heartbeat responses
- ACK/NACK after application validation

## Safety rule

The start-inhibit is a **start prevention** function, not a running-engine kill command. The firmware will not assert the logical inhibit while the engine-running input is active.

If lock is requested while the engine is running, the controller enters `LOCK_PENDING`. In this state:

- start inhibit remains OFF,
- alarm output remains OFF,
- shock warning/trigger cannot start the alarm,
- the controller waits for a confirmed engine-stop condition.

The engine-running filter is intentionally asymmetric: a running indication is accepted immediately, while an engine-stop indication must remain stable for 250 ms before the controller can transition from `LOCK_PENDING` to `LOCKED`. This biases uncertain input behavior toward leaving start prevention disabled.

The final relay/driver wiring must preserve this property electrically as well as in software.

## Shock filtering and alarm timing

Current software defaults:

| Function | Default |
|---|---:|
| Shock warning persistence | 50 ms |
| Shock trigger persistence | 100 ms |
| Engine-stop confirmation | 250 ms |
| Maximum alarm duration | 30 s |

A shock warning or trigger must persist for its confirmation period before it is accepted. Release is immediate so a real new event can be recognized cleanly.

The alarm automatically stops after the configured maximum duration. If the trigger input remains stuck active, it cannot immediately restart the alarm: it must first release and then assert again for the full trigger confirmation period.

These values are software defaults and should be tuned after the actual sensors and motorcycle vibration environment are tested.

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

These are development defaults only and must be checked against the final PCB/harness. Vehicle-level inputs must be conditioned and protected; do not connect motorcycle 12 V directly to ESP32 GPIO.

## Fob support

Fob authentication is intentionally not implemented yet. Protocol IDs are reserved for `AUTH_REQUEST`, `AUTH_RESULT` and `FOB_PRESENT` so a future fob receiver can feed the same local security state machine without redesigning lock/unlock behavior.

## Reset and persistence behavior

Only the intended armed/disarmed state is persisted. Alarm-output state and start-inhibit-output state are never restored directly from storage.

On every ESP32 boot, the hardware layer first forces both alarm and start-inhibit outputs inactive. The NVS-backed armed flag is then read:

- stored disarmed, missing, or unreadable state -> start `UNLOCKED`, outputs remain OFF;
- stored armed state -> start `LOCK_PENDING`, outputs remain OFF.

For an armed recovery, the firmware deliberately treats the engine as potentially running until the engine-running input has been observed stopped continuously for the normal 250 ms confirmation period. Only after that confirmation can the state become `LOCKED` and start prevention be asserted.

If the engine-running input is active after reboot, the controller remains `LOCK_PENDING` indefinitely and start prevention remains OFF. This prevents a reset, brownout, corrupt boot sequence, or stale NVS value from becoming a running-engine kill condition.
