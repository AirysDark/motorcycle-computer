// PlatformIO builds sources from this firmware directory. Pull the portable
// shared implementation into this translation unit so the exact same protocol
// and runtime code used by Linux tests is compiled into the ESP32 firmware.

#include "../../../protocol/src/codec.cpp"
#include "../../../library/src/stream_parser.cpp"
#include "../../../library/src/node.cpp"
#include "../../../library/src/lighting.cpp"
#include "../../../library/src/lighting_server.cpp"
#include "../../../library/src/lighting_controller.cpp"
#include "../../../library/src/lighting_local_control.cpp"
#include "../../../library/src/lighting_diagnostics.cpp"
#include "../../../library/src/lighting_fault_manager.cpp"
