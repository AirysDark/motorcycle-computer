#include <Arduino.h>
#include "bike/lighting_controller.hpp"
#include "lighting_platform.hpp"

namespace {

HardwareSerial NetworkSerial(2);
lighting_platform::ArduinoSerialTransport transport(NetworkSerial);
lighting_platform::GpioLightingHardware hardware;
bike::BikeNode node(bike::NodeAddress::Lighting, transport);
bike::LightingController controller(node, hardware);

} // namespace

void setup() {
    Serial.begin(115200);
    delay(50);

    hardware.begin();

    NetworkSerial.begin(
        lighting_platform::kNetworkBaud,
        SERIAL_8N1,
        lighting_platform::kNetworkRxPin,
        lighting_platform::kNetworkTxPin);

    node.set_ack_timeout_ms(250);
    node.set_max_retries(3);

    Serial.println("lighting-controller: booted");
}

void loop() {
    controller.service(millis());
    delay(1);
}
