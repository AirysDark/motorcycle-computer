# Diagnostic computer (Raspberry Pi 3 or 4)

This is a **software role**, not a Raspberry Pi model designation. Run `motorcycle_diag` on either a Raspberry Pi 3 or Raspberry Pi 4.

Responsibilities:
- discover network controllers and report health
- inspect packet traffic delivered to the diagnostic node
- record fault and event history
- request state and run controller self-tests
- provide the foundation for a future service UI

The diagnostic node uses protocol address `DiagnosticComputer (0x02)` on both Pi models. It is observational by default; control-changing operations must remain explicit.

## Build and run

On either Pi, install CMake, a C++17 compiler and build tools, then run from the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/motorcycle_diag /dev/serial0
```

The link is 115200 baud, 8-N-1. The device path can be changed for a USB serial adapter. Enable UART, disable its login console, and provide an electrically appropriate connection.

The northbridge must actually route diagnostic-node traffic to this Pi. Connecting a second Pi is not automatically supported by a single point-to-point UART. Packet monitoring cannot observe unrelated unmirrored traffic.

See [role-independent deployment](../README.md).
