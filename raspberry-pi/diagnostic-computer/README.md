# Raspberry Pi 3 Diagnostic Computer

Development and diagnostic workstation for the motorcycle computer network.

Initial responsibilities:
- connect to the ESP32-S3 northbridge over serial
- scan/discover the complete controller network
- show live node health and firmware/protocol identity
- monitor decoded packets and raw packet metadata
- maintain a fault/event log
- request controller state and diagnostics

Planned later:
- firmware build/upload workflow for ESP32 nodes
- configuration editor
- persistent diagnostic logs
- graphical dashboard

The diagnostic computer is observational by default. Commands that change motorcycle state will be explicit rather than occurring as a side effect of monitoring.
