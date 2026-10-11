#include "Config.h"
#include "Ports.h"
#include "Router.h"
net::Ports ports;
net::Router router(ports);
void setup(){
 Serial.begin(115200); // USB CDC debugging; enable USB CDC On Boot if supported.
 ports.begin();
 Serial.println("XVS650 ESP32-S3 Northbridge starting");
}
void loop(){
 router.service();
 static uint32_t last=0;
 if(uint32_t(millis()-last)>=config::STATS_INTERVAL_MS){
   last=millis();
   Serial.printf("network forwarded=%lu dropped=%lu\n",(unsigned long)router.forwarded(),(unsigned long)router.dropped());
 }
}
