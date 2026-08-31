# ESP32-S3 Northbridge Firmware

Central packet router for the custom motorcycle computer network.

## Current physical topology

```text
Raspberry Pi 4 / Pi 3
        |
      UART0
        |
    ESP32-S3
    NORTHBRIDGE
     /      \
 UART1      UART2
   |          |
Lighting   Security
ESP32      ESP32
```

The northbridge does not own lighting or security application behaviour. It validates complete protocol frames through the shared parser and routes them between physical ports.

## Bench pin configuration

These are development defaults only and must be verified against the final board and harness.

| Link | RX | TX | Port |
| --- | ---: | ---: | ---: |
| Raspberry Pi uplink | GPIO 4 | GPIO 5 | 0 |
| Lighting southbridge | GPIO 16 | GPIO 17 | 1 |
| Security southbridge | GPIO 18 | GPIO 21 | 2 |

All three links currently run at 115200 baud, 8-N-1.

## Initial routes

- Main computer -> Pi uplink
- Diagnostic computer -> Pi uplink
- Lighting -> lighting port
- Security -> security port

The router also learns source addresses from valid incoming packets, so a node route can follow the physical port where that node is actually observed.

## Routing behaviour

- Known unicast packets are forwarded only to the destination port.
- Broadcast packets are copied to every attached port except the ingress port.
- Packets addressed to the northbridge itself are not forwarded.
- Unknown unicast destinations are dropped instead of flooded.
- Each learned route records `last_seen_ms` for online/offline supervision.

## UART0 development caveat

This first bench design uses ESP32-S3 UART0 for the Raspberry Pi uplink. ESP32 boot/debug output can therefore appear on that physical link during reset. The motorcycle protocol stream parser searches for the two-byte sync marker and can recover after unrelated bytes, but the final hardware may choose a USB/CDC, SPI, or other dedicated Pi uplink to keep UART0 free for service/debug use.

## Build

From this directory:

```bash
pio run
```

Upload with the PlatformIO upload target appropriate for the connected ESP32-S3 board.
