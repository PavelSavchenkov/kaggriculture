#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::joint_base_goose {
class Agent:public joint_day_core::AgentCore {
public:
    Agent():AgentCore(compiler_labor_data::cold_farm(0,0,6,12,0,0),Support{},false,false,false,1,true,3,1,1) {}
    static kag::agent::AgentInfo info() {return {"joint_base_goose"};}
};
}
