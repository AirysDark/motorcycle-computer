# Raspberry Pi deployment: choose a role, not a board model

Both software roles are designed to compile and run on **Raspberry Pi 3 or Raspberry Pi 4** with a supported Linux OS and C++17 toolchain.

| Main computer | Diagnostic computer |
| --- | --- |
| Pi 3 | Pi 3 |
| Pi 3 | Pi 4 |
| Pi 4 | Pi 3 |
| Pi 4 | Pi 4 |

Build both binaries from the same repository on either model:

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
- Compile natively on the destination board (or use a matching cross-compilation toolchain); a binary built for one OS/architecture is not automatically portable to another.
- Configure serial device permissions, UART settings, power supply, display, and startup services for the specific installation.
- Two Raspberry Pis require **two supported physical communication paths or a correctly engineered multiplexed transport** to the ESP32-S3 northbridge; a UART is not a multi-drop bus by itself.
- Edge controllers must continue their autonomous safety-critical operation regardless of Pi model or software availability.

Current software targets Linux/POSIX; this is not a claim of support for arbitrary operating systems. Hardware validation and performance testing on both Pi models are still required.
