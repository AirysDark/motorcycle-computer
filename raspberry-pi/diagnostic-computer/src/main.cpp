#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <poll.h>
#include <string>
#include "bike/node.hpp"
#include "bike/supervisor.hpp"
#include "posix_serial_transport.hpp"
namespace {
std::uint32_t now_ms(){using namespace std::chrono;return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());}
const char* health(bike::NodeHealth h){switch(h){case bike::NodeHealth::Unknown:return"UNKNOWN";case bike::NodeHealth::Online:return"ONLINE";case bike::NodeHealth::Degraded:return"DEGRADED";case bike::NodeHealth::Offline:return"OFFLINE";case bike::NodeHealth::Incompatible:return"INCOMPATIBLE";}return"?";}
void inventory(const bike::NetworkSupervisor&s){std::cout<<"\nNETWORK INVENTORY required-fault="<<(s.required_node_fault()?"YES":"NO")<<'\n';for(const auto&n:s.nodes()){if(!n.discovered)continue;std::cout<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(bike::to_u8(n.address))<<std::dec<<std::setfill(' ')<<"  "<<health(n.health)<<"  required="<<(n.required?"yes":"no");if(n.identity_valid)std::cout<<"  fw="<<unsigned(n.firmware_major)<<'.'<<unsigned(n.firmware_minor)<<'.'<<unsigned(n.firmware_patch)<<" proto="<<unsigned(n.protocol_version)<<" uptime="<<n.uptime_ms<<"ms reboots="<<n.reboot_count<<" faults="<<n.fault_count;else std::cout<<"  identity=pending";std::cout<<'\n';}}
void packet_line(const bike::Packet&p){std::cout<<"RX src=0x"<<std::hex<<unsigned(bike::to_u8(p.source))<<" dst=0x"<<unsigned(bike::to_u8(p.destination))<<" type=0x"<<unsigned(static_cast<std::uint8_t>(p.type))<<std::dec<<" seq="<<p.sequence<<" len="<<p.length<<" flags=0x"<<std::hex<<unsigned(p.flags)<<std::dec<<'\n';}
}
int main(int argc,char**argv){const std::string dev=argc>1?argv[1]:"/dev/serial0";PosixSerialTransport transport;if(!transport.open_device(dev,115200)){std::cerr<<"Unable to open "<<dev<<'\n';return 1;}bike::BikeNode node(bike::NodeAddress::DiagnosticComputer,transport);bike::NetworkSupervisor supervisor(node);supervisor.send_discovery(now_ms());std::cout<<"Motorcycle diagnostic computer on "<<dev<<"\nCommands: scan, inventory, monitor on, monitor off, quit\n> "<<std::flush;bool running=true,monitor=true;pollfd input{};input.fd=0;input.events=POLLIN;std::string line;while(running){const auto now=now_ms();bike::Packet packet{};while(node.poll(packet,now)){supervisor.observe(packet,now);if(monitor)packet_line(packet);}supervisor.service(now);node.service(now);int ready=::poll(&input,1,10);if(ready>0&&(input.revents&POLLIN)){if(!std::getline(std::cin,line))break;if(line=="quit"||line=="exit")running=false;else if(line=="scan"){supervisor.send_discovery(now);std::cout<<"Discovery broadcast sent\n";}else if(line=="inventory")inventory(supervisor);else if(line=="monitor on"){monitor=true;std::cout<<"Packet monitor ON\n";}else if(line=="monitor off"){monitor=false;std::cout<<"Packet monitor OFF\n";}else if(!line.empty())std::cout<<"Unknown command\n";if(running)std::cout<<"> "<<std::flush;}}return 0;}
