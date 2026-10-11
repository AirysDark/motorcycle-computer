#pragma once
#include "Ports.h"
namespace net {
class Router {
public:
 explicit Router(Ports& ports):ports_(ports){}
 void service();
 uint32_t forwarded()const{return forwarded_;}
 uint32_t dropped()const{return dropped_;}
private:
 Ports& ports_;uint32_t forwarded_=0,dropped_=0;uint16_t sequence_=1;
 void route(uint8_t ingress,const Packet& p);
 void local(uint8_t ingress,const Packet& p);
 void reply(uint8_t ingress,uint8_t dst,uint8_t type,uint16_t seq);
};
}
