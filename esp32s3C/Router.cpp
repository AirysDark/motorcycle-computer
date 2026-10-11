#include "Router.h"
namespace net {
void Router::reply(uint8_t port,uint8_t dst,uint8_t type,uint16_t seq){Packet p{};p.src=NORTH;p.dst=dst;p.type=type;p.flags=0x02;p.seq=seq?seq:sequence_++;if(!ports_.send(port,p))++dropped_;}
void Router::local(uint8_t ingress,const Packet& p){
 if(p.flags&0x01)reply(ingress,p.src,ACK,p.seq);
 if(p.type==HEARTBEAT){reply(ingress,p.src,HEARTBEAT,0);return;}
 if(p.type==DISCOVERY){ // Minimal device identity response; full payload implementation is next.
   Packet r{};r.src=NORTH;r.dst=p.src;r.type=DEVICE_INFO;r.flags=0x02;r.seq=sequence_++;
   // Deliberately do not send an invalid DeviceInfo payload.
   return;
 }
 if(p.type==DIAGNOSTIC||p.type==GET_STATE){
   Packet r{};r.src=NORTH;r.dst=p.src;r.type=DIAGNOSTIC;r.flags=0x02;r.seq=sequence_++;r.len=18;
   r.payload[0]=0x02;r.payload[1]=PORTS;
   auto put32=[&r](uint8_t at,uint32_t v){r.payload[at]=uint8_t(v>>24);r.payload[at+1]=uint8_t(v>>16);r.payload[at+2]=uint8_t(v>>8);r.payload[at+3]=uint8_t(v);};
   put32(2,forwarded_);put32(6,dropped_);put32(10,0);put32(14,0);
   // Full port records: 21 bytes each, compatible with host decoder.
   for(uint8_t i=0;i<PORTS;++i){uint8_t at=18+i*21;r.payload[at]=i;put32(at+1,ports_.rx(i));put32(at+5,ports_.tx(i));put32(at+9,0);put32(at+13,0);put32(at+17,ports_.malformed(i));}
   r.len=18+PORTS*21;if(!ports_.send(ingress,r))++dropped_;
 }
}
void Router::route(uint8_t ingress,const Packet& p){
 if(p.src==NORTH||p.src==BROADCAST){++dropped_;return;}
 // Fixed source-to-port bindings prevent a peripheral spoofing the Pi.
 const uint8_t sourcePort=p.src==MAIN?0:p.src==LIGHT?1:p.src==SECURITY?2:255;
 if(sourcePort!=ingress){++dropped_;return;}
 if(p.dst==NORTH){local(ingress,p);return;}
 if(p.dst==BROADCAST){local(ingress,p);for(uint8_t i=0;i<PORTS;++i)if(i!=ingress){if(ports_.send(i,p))++forwarded_;else ++dropped_;}return;}
 const uint8_t target=p.dst==MAIN?0:p.dst==LIGHT?1:p.dst==SECURITY?2:255;
 if(target==255||target==ingress){++dropped_;return;}
 if(ports_.send(target,p))++forwarded_;else ++dropped_;
}
void Router::service(){for(uint8_t port=0;port<PORTS;++port){Packet p{};uint8_t budget=8;while(budget--&&ports_.receive(port,p))route(port,p);}}
}
