#pragma once
#include <Arduino.h>
namespace net {
constexpr uint8_t SYNC1=0xA5,SYNC2=0x5A,VERSION=1,MAIN=0x01,NORTH=0x10,LIGHT=0x20,SECURITY=0x21,BROADCAST=0xFF;
constexpr size_t MAX_PAYLOAD=128, MAX_FRAME=140;
enum Message : uint8_t { HEARTBEAT=0x01, ACK=0x02, GET_STATE=0x10, DEVICE_INFO=0x50, DISCOVERY=0x51, DIAGNOSTIC=0x60 };
struct Packet { uint8_t src=0,dst=0,type=0,flags=0; uint16_t seq=0; uint8_t len=0; uint8_t payload[MAX_PAYLOAD]{}; };
uint16_t crc16(const uint8_t* data,size_t length);
bool encode(const Packet& packet,uint8_t* frame,size_t capacity,size_t& written);
class Parser {
public:
 bool push(uint8_t byte,Packet& packet);
 uint32_t rejected() const {return rejected_;}
private:
 uint8_t buffer_[MAX_FRAME]{}; size_t used_=0,expected_=0; uint32_t rejected_=0;
 void reset(){used_=0;expected_=0;}
};
}
