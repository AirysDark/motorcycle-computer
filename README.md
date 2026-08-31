# Motorcycle Computer

Custom distributed motorcycle computer system for the Yamaha XVS650 project.

## Architecture

- Raspberry Pi 4 — main/supervisory computer
- Raspberry Pi 3 — development and diagnostic computer
- ESP32-S3 — northbridge / central network and routing controller
- ESP32-WROOM #1 — lighting southbridge / I/O controller
- ESP32-WROOM #2 — security southbridge / I/O controller

The project uses a custom communication protocol and shared library rather than CAN. The protocol will provide addressing, routing, message types, sequence numbers, acknowledgements, timeouts, fault reporting and CRC validation across the Raspberry Pi and ESP32 controllers.

## Initial repository layout

- `protocol/` — shared wire protocol definitions
- `library/` — portable communication library
- `esp32/northbridge/` — ESP32-S3 network controller firmware
- `esp32/lighting/` — lighting controller firmware
- `esp32/security/` — security controller firmware
- `raspberry-pi/main-computer/` — Raspberry Pi 4 software
- `raspberry-pi/diagnostics/` — Raspberry Pi 3 diagnostic tools
- `docs/` — architecture and protocol documentation
