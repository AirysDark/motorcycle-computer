#pragma once
#include <string>
#include "bike_protocol/messages.hpp"
namespace bikehost {
enum class Role { Main, Diagnostic };
struct Config { Role role{Role::Diagnostic}; std::string device{"/dev/serial0"}; };
inline bool parse_role(const std::string& s,Role& r){if(s=="main"){r=Role::Main;return true;}if(s=="diagnostic"){r=Role::Diagnostic;return true;}return false;}
inline bike::NodeAddress address(Role r){return r==Role::Main?bike::NodeAddress::MainComputer:bike::NodeAddress::DiagnosticComputer;}
inline bool parse_arguments(int argc,char** argv,Role default_role,Config& config,std::string& error){config.role=default_role;for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--role"){if(i+1>=argc||!parse_role(argv[++i],config.role)){error="--role requires main or diagnostic";return false;}}else if(arg=="--device"){if(i+1>=argc||std::string(argv[i+1]).empty()){error="--device requires a path";return false;}config.device=argv[++i];}else if(arg=="--help"){error="usage: --role main|diagnostic --device /dev/serial0";return false;}else if(arg.rfind("--",0)==0){error="unknown option: "+arg;return false;}else if(i==1){config.device=arg;}else{error="unexpected argument: "+arg;return false;}}return true;}
}
