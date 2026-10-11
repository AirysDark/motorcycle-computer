# Raspberry Pi deployment: choose a role, not a board model

Both software roles are designed to compile and run on **Raspberry Pi 3 or Raspberry Pi 4** with supported 32-bit (armhf) or 64-bit (arm64) Raspberry Pi Linux installations and a C++17 toolchain.

| Main computer | Diagnostic computer |
| --- | --- |
| Pi 3 | Pi 3 |
| Pi 3 | Pi 4 |
| Pi 4 | Pi 3 |
| Pi 4 | Pi 4 |

Build both binaries from the same repository on either model and OS architecture:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

Select the role when starting the software:

```sh
./build/motorcycle_main /dev/serial0
# OR
./build/motorcycle_diag /dev/serial0
```

- Main computer: protocol node `0x01`.
- Diagnostic computer: protocol node `0x02`.
- Run only one instance per role/address on the same network.
- The Pi board type does not change protocol addresses or the ESP32 firmware.
- Both applications are intended to support **32-bit ARM Linux (armhf)** and **64-bit ARM Linux (arm64)**. Build natively for the installed OS, or cross-compile with a matching toolchain. A 64-bit executable does not run on a 32-bit OS, and a 32-bit executable on a 64-bit OS requires compatible 32-bit runtime libraries.\n- Check architecture using `uname -m` and `getconf LONG_BIT`; compile and test separately for each target architecture.
- Configure serial device permissions, UART settings, power supply, display, and startup services for the specific installation.
- Two Raspberry Pis require **two supported physical communication paths or a correctly engineered multiplexed transport** to the ESP32-S3 northbridge; a UART is not a multi-drop bus by itself.
- Edge controllers must continue their autonomous safety-critical operation regardless of Pi model or software availability.

Current software targets Linux/POSIX; this is not a claim of support for arbitrary operating systems. Hardware validation and performance testing on both Pi models are still required.

## Replacing a failed Pi

A replacement Raspberry Pi 3 can take over from a failed Raspberry Pi 4, and vice versa, **without changing application source code or ESP32 firmware**. Install the same repository on the replacement Pi, build it for that Pi's Linux OS, and run the same role binary. The node address is determined by the executable role, not the board model.

1. Power down the motorcycle electronics and disconnect the failed Pi.
2. Install a compatible Raspberry Pi OS on the replacement Pi and connect the configured, protected serial interface.
3. Build both applications using the commands above.
4. Restore any backed-up application configuration and verify serial device naming and permissions.
5. Run `motorcycle_main` if replacing the main computer, or `motorcycle_diag` if replacing the diagnostic computer.
6. Confirm controller discovery, node identity, and correct operation before using the motorcycle.

Do not connect two active main-role instances: both would claim address `0x01`. The replacement must use the same role but can be a different Pi model. A replacement may need display, UART and startup-service configuration specific to its OS installation.

**Limitations:** Portable application source is not the same as an interchangeable SD card image. Pi OS images, boot firmware, device overlays and hardware-specific settings can differ. The project does not yet provide a one-command installer, automatic restoration of settings, or validated boot-time failover.

## 32-bit and 64-bit compatibility

The codebase uses C++17 and POSIX serial APIs and does not intentionally require 64-bit-only Linux interfaces. Supported deployment targets are Raspberry Pi 3 or 4 running either 32-bit or 64-bit Raspberry Pi Linux, provided the compiler, dependencies and serial device are configured for that OS.

**Compatibility target, not a completed test certification:** successful native builds and runtime tests on both armhf and arm64 remain to be verified. Keep distinct builds/packages for each architecture; do not copy an arm64 executable onto a 32-bit installation.
