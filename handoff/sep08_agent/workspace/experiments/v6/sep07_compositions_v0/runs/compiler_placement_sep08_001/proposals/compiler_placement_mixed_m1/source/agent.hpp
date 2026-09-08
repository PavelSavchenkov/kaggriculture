#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_placement_mixed_m1 {
class Agent:public compiler_placement_sep08::AgentCore {
public:
    Agent():AgentCore(compiler_labor_data::cold_farm(2,2,0,7,12,8),Support{},false,false,false,1,true,1,1) {}
    static kag::agent::AgentInfo info() {return {"compiler_placement_mixed_m1"};}
};
}
