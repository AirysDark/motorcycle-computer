#pragma once
#include <cstdint>
#include <deque>
#include <fstream>
#include <string>
#include "bike_protocol/packet.hpp"
namespace diag {
struct Event { std::uint32_t time_ms{0}; bike::NodeAddress source{bike::NodeAddress::Broadcast}; bike::NodeAddress destination{bike::NodeAddress::Broadcast}; bike::MessageType type{bike::MessageType::Heartbeat}; std::string text; };
class DiagnosticLogger {
public:
    explicit DiagnosticLogger(std::size_t limit=250):limit_(limit){}
    bool open(const std::string& path);
    void record(std::uint32_t now_ms,const bike::Packet& packet,const std::string& decoded);
    void clear(){events_.clear();}
    const std::deque<Event>& events() const{return events_;}
private:
    std::size_t limit_; std::deque<Event> events_; std::ofstream file_;
};
} // namespace diag
