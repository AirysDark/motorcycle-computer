# Motorcycle Computer Protocol v1

Status: initial development specification.

## Purpose

Protocol v1 is the common binary language between Raspberry Pi computers, the ESP32-S3 northbridge and ESP32 southbridge/I/O controllers. Application code should use the shared library instead of manually assembling frames.

## Frame layout

All multi-byte integers are transmitted big-endian.

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | Sync byte 1 (`0xA5`) |
| 1 | 1 | Sync byte 2 (`0x5A`) |
| 2 | 1 | Protocol version (`0x01`) |
| 3 | 1 | Source address |
| 4 | 1 | Destination address |
| 5 | 1 | Message type |
| 6 | 1 | Flags |
| 7 | 2 | Sequence number |
| 9 | 1 | Payload length |
| 10 | 0..128 | Payload |
| variable | 2 | CRC-16/CCITT-FALSE |

CRC covers the complete frame from the first sync byte through the final payload byte. The CRC itself is not included in the calculation.

## Node addresses

| Address | Node |
|---:|---|
| `0x01` | Raspberry Pi 4 main computer |
| `0x02` | Raspberry Pi 3 diagnostic computer |
| `0x10` | ESP32-S3 northbridge |
| `0x20` | Lighting controller |
| `0x21` | Security controller |
| `0xFF` | Broadcast |

## Message types

| ID | Type |
|---:|---|
| `0x01` | HEARTBEAT |
| `0x02` | ACK |
| `0x03` | NACK |
| `0x10` | GET_STATE |
| `0x11` | STATE_UPDATE |
| `0x20` | SET_OUTPUT |
| `0x21` | INPUT_EVENT |
| `0x30` | CONFIG_READ |
| `0x31` | CONFIG_WRITE |
| `0x40` | FAULT |
| `0x41` | FAULT_CLEAR |
| `0x50` | DEVICE_INFO |
| `0x51` | DEVICE_DISCOVERY |
| `0x60` | DIAGNOSTIC |

## Flags

- `0x01` ACK required
- `0x02` response
- `0x04` fault

## Example

A Pi 4 command telling the lighting controller to switch the left indicator on is represented logically as:

- source: `0x01`
- destination: `0x20`
- type: `SET_OUTPUT`
- ACK required: yes
- payload byte 0: output ID `0x01` (left indicator)
- payload byte 1: value `0x01` (on)

The lighting controller should perform the local operation and return an ACK and/or STATE_UPDATE according to the higher-level command rules as those rules are formalized.

## Next protocol work

Protocol v1 still needs stream framing/resynchronization, ACK/NACK payload definitions, timeout/retry policy, heartbeat timing, device discovery payloads, typed lighting/security payload schemas and router forwarding rules.
