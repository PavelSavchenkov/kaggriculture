#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::compiler_route_p4_m3 {
class Agent:public compiler_route_sep08::AgentCore {
public:
    Agent():AgentCore(4,true,true,true,1,true,0,3) {}
    static kag::agent::AgentInfo info() {return {"compiler_route_p4_m3"};}
};
}
