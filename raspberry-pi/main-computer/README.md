# Main motorcycle computer (Raspberry Pi 3 or 4)

This is a **software role**, not a Raspberry Pi model designation. Run `motorcycle_main` on either a Raspberry Pi 3 or Raspberry Pi 4 running a supported Linux distribution.

Responsibilities:
- supervise the ESP32-S3 northbridge and distributed controllers
- expose lighting and security APIs
- track node discovery, heartbeats, and communication faults
- host the main motorcycle UI without owning safety-critical physical I/O

Lighting and security edge controllers retain local authority if Linux restarts or loses its connection.

## Build on either Pi

Install CMake, a C++17 compiler and build tools on the Pi, then from the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

Run:

```sh
./build/motorcycle_main /dev/serial0
```

The serial link is 115200 baud, 8-N-1; specify a different Linux device path if using a USB serial adapter. Enable the serial port and disable its login console in Raspberry Pi configuration before using UART. Use the appropriate 3.3 V-safe interface and electrical protection for the motorcycle harness.

The program uses protocol address `MainComputer (0x01)` on both Pi models. Do not run two main-role instances on the same network.

## Choosing hardware

Pi 3 and Pi 4 are both intended targets. The Pi 4 has more processing headroom for graphical interfaces; the Pi 3 can run the supervisory application. Hardware interfaces, serial port configuration, display support, and actual performance must be verified on each installation.

See [role-independent deployment](../README.md).
