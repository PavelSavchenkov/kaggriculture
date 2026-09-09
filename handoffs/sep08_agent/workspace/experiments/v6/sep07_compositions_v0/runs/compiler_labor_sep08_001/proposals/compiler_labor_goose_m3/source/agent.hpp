#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::compiler_labor_goose_m3 {
class Agent:public compiler_labor_sep08::AgentCore {
public:
    Agent():AgentCore(compiler_labor_data::cold_farm(0,0,6,12,0,0),Support{},false,false,false,1,true,3) {}
    static kag::agent::AgentInfo info() {return {"compiler_labor_goose_m3"};}
};
}
