#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::compiler_labor_p4_m2 {
class Agent:public compiler_labor_sep08::AgentCore {
public:
    Agent():AgentCore(4,true,false,true,1,true,2) {}
    static kag::agent::AgentInfo info() {return {"compiler_labor_p4_m2"};}
};
}
