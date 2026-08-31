# Raspberry Pi 4 Main Computer

Linux supervisory application for the motorcycle computer.

## Responsibilities

- communicate with the ESP32-S3 northbridge
- expose typed lighting and security APIs
- track node discovery, heartbeat state and communication faults
- provide the UI/application layer without owning safety-critical physical I/O

The Raspberry Pi is supervisory. Lighting and security controllers retain local authority for their hardware functions if Linux restarts or the link is lost.

## Serial link

The current runtime uses a POSIX serial device at 115200 baud, 8-N-1. The default device is:

```text
/dev/serial0
```

A different device can be passed on the command line.

## Build

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

The executable is `motorcycle_main`.

## Run

```bash
./build/motorcycle_main /dev/serial0
```

Initial console commands:

```text
status
refresh
left on
left off
right on
right off
brake on
brake off
high on
high off
lock
unlock
silence
quit
```

The CLI is intentionally small. It exercises the same high-level API that the future graphical UI will use.
