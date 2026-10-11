# XVS650 ESP32-S3 Northbridge (Arduino IDE)

This folder is a standalone Arduino IDE sketch. Open `esp32s3C/esp32s3C.ino` in Arduino IDE with the Espressif ESP32 board package installed and select your **actual ESP32-S3 board**. All `.h` and `.cpp` files remain in this sketch folder for straightforward debugging.

## Files
- `Config.h` — UART baud and GPIO assignment placeholders
- `Protocol.h/.cpp` — host-compatible A5 5A framed packets, CRC16-CCITT-FALSE, stream parsing
- `Ports.h/.cpp` — independent hardware UART links
- `Router.h/.cpp` — fixed-address forwarding, broadcast, heartbeat, basic network diagnostics
- `esp32s3C.ino` — startup and USB debug output

## Wiring and setup
**Do not wire this to the motorcycle yet.** GPIO pin numbers in `Config.h` are provisional; confirm against your exact S3 board pinout and boot/USB/JTAG reservations. UART is **3.3 V TTL**, not RS-232 and not 12 V. Use a common signal ground with appropriately protected/isolated power arrangements. Cross TX to RX on each point-to-point link. The three serial links are main Pi (port 0), lighting ESP32 (port 1), security ESP32 (port 2). Serial debug is USB CDC; UART0 is assigned to the security link, so do not rely on UART0 for serial debugging.

Protocol is the repository's **custom UART protocol, not CAN**: 115200 8N1, sync A5 5A, version 1, 10-byte header, up to 128 payload bytes, 2-byte big-endian CRC16-CCITT-FALSE. Fixed node addresses are 0x01 main Pi, 0x10 S3, 0x20 lighting, 0x21 security. One Pi only; diagnostics are integrated into the Pi EMU.

## Status and limitations
Initial communication implementation, **not yet compiled on an ESP32-S3 or hardware-tested**. This version forwards packets, validates frames and source ports, answers heartbeats and sends northbridge diagnostics. Device discovery's DeviceInfo reply is **not implemented yet**. Port byte counters and topology-fault reporting are also incomplete. Forwarding has no transmit queue: a full UART TX buffer can drop a packet. Safety-critical brake/indicator/security operation must remain autonomous on their dedicated controllers. Never use this prototype to drive safety-critical outputs.
