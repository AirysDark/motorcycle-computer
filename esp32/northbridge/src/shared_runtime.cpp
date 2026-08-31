// PlatformIO builds sources from this firmware directory. Pull the portable
// shared implementation into this translation unit so Linux tests and ESP32-S3
// firmware compile the same protocol and routing code.

#include "../../../protocol/src/codec.cpp"
#include "../../../library/src/stream_parser.cpp"
#include "../../../library/src/router.cpp"
#include "../../../library/src/northbridge.cpp"
