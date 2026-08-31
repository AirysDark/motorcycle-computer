#include <Arduino.h>
#include "bike/lighting_controller.hpp"
#include "bike/lighting_diagnostics.hpp"
#include "bike/lighting_fault_manager.hpp"
#include "bike/lighting_local_control.hpp"
#include "lighting_platform.hpp"

namespace {
HardwareSerial NetworkSerial(2);
lighting_platform::ArduinoSerialTransport transport(NetworkSerial);
lighting_platform::GpioLightingHardware hardware;
lighting_platform::GpioLightingInputs inputs;
lighting_platform::NoLightingElectricalFeedback electrical_feedback;
bike::BikeNode node(bike::NodeAddress::Lighting, transport);
bike::LightingController controller(node, hardware);
bike::LightingLocalControl local_control(controller, hardware, inputs);
bike::LightingDiagnostics diagnostics(node, hardware, electrical_feedback);
bike::LightingFaultManager fault_manager(node, diagnostics, hardware);
}

void setup() {
    Serial.begin(115200);
    delay(50);
    hardware.begin();
    inputs.begin();
    NetworkSerial.begin(lighting_platform::kNetworkBaud, SERIAL_8N1,
                        lighting_platform::kNetworkRxPin, lighting_platform::kNetworkTxPin);
    node.set_ack_timeout_ms(250);
    node.set_max_retries(3);
    local_control.set_blink_half_period_ms(500);
    local_control.set_debounce_ms(25);
    diagnostics.set_publish_interval_ms(1000);
    fault_manager.set_confirm_samples(3);
    // All channels remain ReportOnly by default. Automatic shutdown is opt-in
    // after real current-sense hardware and thresholds have been validated.
    Serial.println("lighting-controller: booted");
}

void loop() {
    const auto now_ms = millis();
    controller.service(now_ms);
    local_control.service(now_ms);
    fault_manager.service(now_ms);
    diagnostics.service(now_ms);
    delay(1);
}
