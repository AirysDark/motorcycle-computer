# ESP32-WROOM Lighting Controller

This firmware is the hardware target for the lighting southbridge.

## Responsibilities

- Runs as node address `0x20` (`Lighting`).
- Communicates with the northbridge over UART2 using the shared motorcycle protocol.
- Receives typed lighting commands through `LightingController` / `LightingServer`.
- Reads the physical left/right indicator switches, brake input and high-beam switch locally.
- Runs indicator flashing locally, independent of the Raspberry Pi or northbridge.
- Drives the four switched lighting outputs through `GpioLightingHardware`.
- Starts every switched output in a defined OFF state.
- Responds to discovery, heartbeat, device-info and state requests through the shared controller runtime.
- ACKs valid remote commands only after the application/hardware layer accepts them and NACKs invalid or failed commands.

## Local authority and arbitration

Physical motorcycle controls remain authoritative. The Pi can request outputs, but loss of the Pi or northbridge does not remove basic lighting operation.

- A physical brake input forces the brake-bright circuit ON.
- A physical high-beam input forces the high-beam circuit ON.
- A physical left or right indicator request runs a local 500 ms half-period flasher.
- If both indicator inputs are active, both sides flash together (hazard-like behavior).
- When no local input is active, the output falls back to the most recent remote commanded state.

This split intentionally keeps deterministic motorcycle I/O at the edge rather than making Linux/network availability part of the lighting control path.

## Initial bench pin map

| Function | GPIO |
| --- | ---: |
| Network RX | 16 |
| Network TX | 17 |
| Left switch input | 18 |
| Right switch input | 19 |
| Brake input | 21 |
| High-beam input | 22 |
| Left indicator output | 25 |
| Right indicator output | 26 |
| Brake-bright output | 27 |
| High-beam output | 32 |

The four bench inputs are active-low with internal pull-ups. The final motorcycle interface must not connect 12 V vehicle wiring directly to ESP32 GPIO. Use the appropriate automotive input conditioning/protection for the final harness.

These are **bench defaults**, not a final motorcycle wiring specification. Verify them against the exact ESP32-WROOM board, PCB, boot-strapping constraints and power-stage wiring before connecting vehicle loads.

## UART

Initial bench configuration:

- UART2
- 115200 baud
- 8 data bits
- no parity
- 1 stop bit

## Build

From this directory with PlatformIO installed:

```bash
pio run
```

Upload to a connected generic ESP32 development board:

```bash
pio run --target upload
```

Serial monitor:

```bash
pio device monitor
```

## Source layout

```text
esp32/lighting/
├── platformio.ini
├── include/
│   └── lighting_platform.hpp
└── src/
    ├── main.cpp
    └── shared_runtime.cpp
```

`shared_runtime.cpp` intentionally compiles the portable protocol/runtime sources from the repository root into the firmware, keeping the Linux-tested and ESP32 protocol implementations identical.

## Electrical-state limitation

`GpioLightingHardware::read_output()` currently reports the controller's GPIO-commanded state. It does **not** prove that a lamp is drawing current or that the MOSFET/load is electrically healthy. A later hardware revision should add current/load feedback so the protocol can distinguish commanded state, GPIO state and measured electrical state.
