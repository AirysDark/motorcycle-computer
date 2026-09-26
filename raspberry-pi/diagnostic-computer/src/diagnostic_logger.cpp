#include "diagnostic_logger.hpp"
namespace diag {
namespace { const char* node_name(bike::NodeAddress a){switch(a){case bike::NodeAddress::MainComputer:return"MAIN";case bike::NodeAddress::DiagnosticComputer:return"DIAG";case bike::NodeAddress::Northbridge:return"NORTHBRIDGE";case bike::NodeAddress::Lighting:return"LIGHTING";case bike::NodeAddress::Security:return"SECURITY";case bike::NodeAddress::Broadcast:return"BROADCAST";}return"UNKNOWN";} }
bool DiagnosticLogger::open(const std::string& path){file_.open(path,std::ios::app);if(!file_)return false;if(file_.tellp()==0)file_<<"time_ms,source,destination,type,decoded\n";return true;}
void DiagnosticLogger::record(std::uint32_t now,const bike::Packet&p,const std::string&decoded){events_.push_back({now,p.source,p.destination,p.type,decoded});while(events_.size()>limit_)events_.pop_front();if(file_){std::string escaped=decoded;std::size_t pos=0;while((pos=escaped.find('"',pos))!=std::string::npos){escaped.insert(pos,"\"");pos+=2;}file_<<now<<','<<node_name(p.source)<<','<<node_name(p.destination)<<','<<static_cast<unsigned>(p.type)<<",\""<<escaped<<"\"\n";file_.flush();}}
} // namespace diag
