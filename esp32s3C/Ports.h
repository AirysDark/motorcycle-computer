#pragma once
#include "Protocol.h"
namespace net {
constexpr uint8_t PORTS=3;
class Ports {
public:
 void begin();
 void service();
 bool send(uint8_t port,const Packet& packet);
 bool receive(uint8_t port,Packet& packet);
 uint32_t rx(uint8_t port)const{return port<PORTS?rx_[port]:0;}
 uint32_t tx(uint8_t port)const{return port<PORTS?tx_[port]:0;}
 uint32_t malformed(uint8_t port)const{return port<PORTS?parsers_[port].rejected():0;}
private:
 HardwareSerial* serial_[PORTS]{};
 Parser parsers_[PORTS]{};
 uint32_t rx_[PORTS]{},tx_[PORTS]{};
};
}
