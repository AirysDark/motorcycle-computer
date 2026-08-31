# ESP32-WROOM Lighting Controller

This firmware is the first hardware target for the lighting southbridge.

## Responsibilities

- Runs as node address `0x20` (`Lighting`).
- Communicates with the northbridge over UART2 using the shared motorcycle protocol.
- Receives typed lighting commands through `LightingController` / `LightingServer`.
- Drives the four switched lighting outputs through `GpioLightingHardware`.
- Starts every switched output in a defined OFF state.
- Responds to discovery, heartbeat and device-info requests through the shared controller runtime.
- ACKs valid commands only after the application/hardware layer accepts them and NACKs invalid or failed commands.

## Initial bench pin map

| Function | GPIO |
| --- | ---: |
| Network RX | 16 |
| Network TX | 17 |
| Left indicator | 25 |
| Right indicator | 26 |
| Brake bright | 27 |
| High beam | 32 |

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
