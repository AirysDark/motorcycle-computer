#include "Ports.h"
#include "Config.h"
namespace net {
void Ports::begin(){static HardwareSerial mainUart(1),lightUart(2);
// ESP32-S3 has three hardware UART controllers; Arduino USB CDC is used for debug.
static HardwareSerial securityUart(0);
serial_[0]=&mainUart;serial_[1]=&lightUart;serial_[2]=&securityUart;
serial_[0]->begin(config::BAUD,SERIAL_8N1,config::MAIN_RX,config::MAIN_TX);
serial_[1]->begin(config::BAUD,SERIAL_8N1,config::LIGHT_RX,config::LIGHT_TX);
serial_[2]->begin(config::BAUD,SERIAL_8N1,config::SECURITY_RX,config::SECURITY_TX);
}
void Ports::service(){}
bool Ports::send(uint8_t port,const Packet& p){if(port>=PORTS||!serial_[port])return false;uint8_t frame[MAX_FRAME];size_t n=0;if(!encode(p,frame,sizeof(frame),n))return false;if(serial_[port]->availableForWrite()<int(n))return false;size_t written=serial_[port]->write(frame,n);if(written==n)++tx_[port];return written==n;}
bool Ports::receive(uint8_t port,Packet& p){if(port>=PORTS||!serial_[port])return false;uint16_t budget=256;while(budget--&&serial_[port]->available()){if(parsers_[port].push(uint8_t(serial_[port]->read()),p)){++rx_[port];return true;}}return false;}
}
