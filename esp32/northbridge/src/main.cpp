#include <Arduino.h>

#include "bike/northbridge.hpp"
#include "bike_protocol/addresses.hpp"
#include "northbridge_platform.hpp"

using northbridge_platform::ArduinoSerialTransport;

HardwareSerial PiSerial(0);
HardwareSerial LightingSerial(1);
HardwareSerial SecuritySerial(2);

ArduinoSerialTransport pi_transport(PiSerial);
ArduinoSerialTransport lighting_transport(LightingSerial);
ArduinoSerialTransport security_transport(SecuritySerial);

bike::NorthbridgeRuntime northbridge;

void setup() {
    PiSerial.begin(
        northbridge_platform::kNetworkBaud,
        SERIAL_8N1,
        northbridge_platform::kPiRxPin,
        northbridge_platform::kPiTxPin);

    LightingSerial.begin(
        northbridge_platform::kNetworkBaud,
        SERIAL_8N1,
        northbridge_platform::kLightingRxPin,
        northbridge_platform::kLightingTxPin);

    SecuritySerial.begin(
        northbridge_platform::kNetworkBaud,
        SERIAL_8N1,
        northbridge_platform::kSecurityRxPin,
        northbridge_platform::kSecurityTxPin);

    northbridge.attach_port(northbridge_platform::kPortPi, pi_transport);
    northbridge.attach_port(northbridge_platform::kPortLighting, lighting_transport);
    northbridge.attach_port(northbridge_platform::kPortSecurity, security_transport);

    // Seed known topology. Source learning can still update a route if a node
    // is physically moved to a different port during development.
    northbridge.set_route(bike::NodeAddress::MainComputer, northbridge_platform::kPortPi);
    northbridge.set_route(bike::NodeAddress::DiagnosticComputer, northbridge_platform::kPortPi);
    northbridge.set_route(bike::NodeAddress::Lighting, northbridge_platform::kPortLighting);
    northbridge.set_route(bike::NodeAddress::Security, northbridge_platform::kPortSecurity);
}

void loop() {
    northbridge.service(millis());
}
