#include "Protocol.h"
namespace net {
uint16_t crc16(const uint8_t* data,size_t n){uint16_t crc=0xFFFF;for(size_t i=0;i<n;++i){crc^=uint16_t(data[i])<<8;for(uint8_t b=0;b<8;++b)crc=(crc&0x8000)?uint16_t((crc<<1)^0x1021):uint16_t(crc<<1);}return crc;}
bool encode(const Packet& p,uint8_t* f,size_t cap,size_t& written){written=0;if(p.len>MAX_PAYLOAD||cap<size_t(12)+p.len)return false;f[0]=SYNC1;f[1]=SYNC2;f[2]=VERSION;f[3]=p.src;f[4]=p.dst;f[5]=p.type;f[6]=p.flags;f[7]=uint8_t(p.seq>>8);f[8]=uint8_t(p.seq);f[9]=p.len;for(size_t i=0;i<p.len;++i)f[10+i]=p.payload[i];uint16_t crc=crc16(f,10+p.len);f[10+p.len]=uint8_t(crc>>8);f[11+p.len]=uint8_t(crc);written=12+p.len;return true;}
bool Parser::push(uint8_t b,Packet& p){
 if(used_==0){if(b!=SYNC1)return false;buffer_[used_++]=b;return false;}
 if(used_==1){if(b!=SYNC2){reset();if(b==SYNC1)buffer_[used_++]=b;return false;}buffer_[used_++]=b;return false;}
 if(used_>=MAX_FRAME){++rejected_;reset();return false;}
 buffer_[used_++]=b;
 if(used_==10){if(buffer_[2]!=VERSION||buffer_[9]>MAX_PAYLOAD){++rejected_;reset();return false;}expected_=12+buffer_[9];}
 if(expected_&&used_==expected_){const size_t n=10+buffer_[9];const uint16_t got=(uint16_t(buffer_[n])<<8)|buffer_[n+1];if(crc16(buffer_,n)!=got){++rejected_;reset();return false;}p.src=buffer_[3];p.dst=buffer_[4];p.type=buffer_[5];p.flags=buffer_[6];p.seq=(uint16_t(buffer_[7])<<8)|buffer_[8];p.len=buffer_[9];for(size_t i=0;i<p.len;++i)p.payload[i]=buffer_[10+i];reset();return true;}
 return false;
}
}
