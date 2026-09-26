#include "bike/supervisor.hpp"
namespace bike { namespace { std::uint32_t get_u32(const std::uint8_t* p){return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];} }
NetworkSupervisor::NetworkSupervisor(BikeNode& node):node_(node){set_required(NodeAddress::Northbridge);set_required(NodeAddress::Lighting);set_required(NodeAddress::Security);}
NodeStatus* NetworkSupervisor::find_or_create(NodeAddress a){for(auto& e:nodes_)if(e.discovered&&e.address==a)return &e;for(auto& e:nodes_)if(!e.discovered){e.address=a;e.discovered=true;return &e;}return nullptr;}
bool NetworkSupervisor::set_required(NodeAddress a,bool r){auto* e=find_or_create(a);if(!e)return false;e->required=r;update_health(*e);return true;}
const NodeStatus* NetworkSupervisor::status(NodeAddress a)const{for(const auto& e:nodes_)if(e.discovered&&e.address==a)return &e;return nullptr;}
std::size_t NetworkSupervisor::known_node_count()const{std::size_t c=0;for(const auto&e:nodes_)if(e.discovered)++c;return c;}
bool NetworkSupervisor::send_discovery(std::uint32_t now){Packet p{};p.destination=NodeAddress::Broadcast;p.type=MessageType::DeviceDiscovery;if(!node_.send(p,now,false))return false;last_discovery_tx_ms_=now;return true;}
bool NetworkSupervisor::send_heartbeat(std::uint32_t now){Packet p{};p.destination=NodeAddress::Broadcast;p.type=MessageType::Heartbeat;if(!node_.send(p,now,false))return false;last_heartbeat_tx_ms_=now;return true;}
void NetworkSupervisor::update_health(NodeStatus& e){if(!e.online){e.health=e.discovered?NodeHealth::Offline:NodeHealth::Unknown;return;}if(e.identity_valid&&!e.protocol_compatible){e.health=NodeHealth::Incompatible;return;}if(e.fault_count>0){e.health=NodeHealth::Degraded;return;}e.health=NodeHealth::Online;}
void NetworkSupervisor::observe(const Packet& p,std::uint32_t now){auto* e=find_or_create(p.source);if(!e)return;e->online=true;e->last_seen_ms=now;
 if(p.type==MessageType::DeviceInfo&&p.length>=kDeviceInfoPayloadSize&&p.payload[0]==kDeviceInfoSchemaVersion){const std::uint32_t new_uptime=get_u32(&p.payload[10]);if(e->identity_valid&&new_uptime<e->uptime_ms)++e->reboot_count;e->identity_schema=p.payload[0];e->address=static_cast<NodeAddress>(p.payload[1]);e->device_class=static_cast<DeviceClass>(p.payload[2]);e->protocol_version=p.payload[3];e->firmware_major=p.payload[4];e->firmware_minor=p.payload[5];e->firmware_patch=p.payload[6];e->hardware_revision=p.payload[7];e->reset_reason=static_cast<ResetReason>(p.payload[8]);e->uptime_ms=new_uptime;e->capabilities=std::uint16_t((std::uint16_t(p.payload[14])<<8)|p.payload[15]);e->protocol_compatible=e->protocol_version==kProtocolVersion;e->identity_valid=true;}
 else if(p.type==MessageType::Heartbeat)e->last_heartbeat_ms=now;else if(p.type==MessageType::Fault)++e->fault_count;update_health(*e);}
void NetworkSupervisor::service(std::uint32_t now){if(std::uint32_t(now-last_heartbeat_tx_ms_)>=heartbeat_interval_ms_)send_heartbeat(now);bool missing_required=false;
 for(auto& e:nodes_){if(!e.discovered)continue;if(e.online&&std::uint32_t(now-e.last_seen_ms)>=offline_timeout_ms_)e.online=false;update_health(e);if(e.required&&(!e.online||e.health==NodeHealth::Incompatible))missing_required=true;}
 required_node_fault_=missing_required;if(missing_required&&std::uint32_t(now-last_discovery_tx_ms_)>=rediscovery_interval_ms_)send_discovery(now);}
} // namespace bike
