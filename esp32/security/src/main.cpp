#include <Arduino.h>
#include "bike/node.hpp"
#include "bike/security_controller.hpp"
#include "security_platform.hpp"

HardwareSerial NetworkSerial(2);
security_platform::ArduinoSerialTransport transport(NetworkSerial);
bike::BikeNode node(bike::NodeAddress::Security, transport);
security_platform::GpioSecurityHardware hardware;
bike::SecurityController controller(node, hardware);

void setup() {
    hardware.begin();

    NetworkSerial.begin(
        security_platform::kNetworkBaud,
        SERIAL_8N1,
        security_platform::kNetworkRxPin,
        security_platform::kNetworkTxPin);

    node.set_ack_timeout_ms(250);
    node.set_max_retries(3);
}

void loop() {
    controller.service(millis());
}
