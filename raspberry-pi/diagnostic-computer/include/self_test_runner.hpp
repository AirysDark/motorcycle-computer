#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "bike/node.hpp"
#include "bike/supervisor.hpp"
namespace diag {
enum class TestResult{Pending,Pass,Warn,Fail};
struct TestItem{bike::NodeAddress node;bike::MessageType request;bike::MessageType response;TestResult result{TestResult::Pending};std::uint32_t started{0};std::uint32_t elapsed{0};std::string detail;};
class SelfTestRunner {
public:
    void begin(bike::BikeNode& node,const bike::NetworkSupervisor& supervisor,std::uint32_t now_ms);
    void observe(const bike::Packet& packet,bike::BikeNode& node,const bike::NetworkSupervisor& supervisor,std::uint32_t now_ms);
    void service(bike::BikeNode& node,const bike::NetworkSupervisor& supervisor,std::uint32_t now_ms);
    bool active()const{return active_;} bool report_ready()const{return report_ready_;} void consume_report(){report_ready_=false;}
    std::uint32_t total_elapsed()const{return total_elapsed_;} const std::array<TestItem,3>& items()const{return items_;}
private:
    void start_next(bike::BikeNode&,const bike::NetworkSupervisor&,std::uint32_t); void finish(std::uint32_t);
    bool active_{false},report_ready_{false};std::size_t index_{0};std::uint32_t started_{0},total_elapsed_{0};
    std::array<TestItem,3> items_{{{bike::NodeAddress::Northbridge,bike::MessageType::Diagnostic,bike::MessageType::Diagnostic},{bike::NodeAddress::Lighting,bike::MessageType::GetState,bike::MessageType::StateUpdate},{bike::NodeAddress::Security,bike::MessageType::GetState,bike::MessageType::SecurityState}}};
};
} // namespace diag
