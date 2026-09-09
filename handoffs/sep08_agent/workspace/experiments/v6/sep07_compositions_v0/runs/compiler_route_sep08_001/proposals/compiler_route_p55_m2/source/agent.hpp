#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::compiler_route_p55_m2 {
class Agent:public compiler_route_sep08::AgentCore {
public:
    Agent():AgentCore(55,true,true,true,1,true,0,2) {}
    static kag::agent::AgentInfo info() {return {"compiler_route_p55_m2"};}
};
}
