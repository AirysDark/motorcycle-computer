# Motorcycle Communication Protocol

This directory defines the common binary protocol used by Raspberry Pi and ESP32 nodes.

Planned frame fields:

`SYNC | VERSION | SOURCE | DESTINATION | MESSAGE_TYPE | FLAGS | SEQUENCE | LENGTH | PAYLOAD | CRC`

Initial message classes will include heartbeat, ACK/NACK, commands, state updates, input events, faults, configuration, device discovery/info and diagnostics.
