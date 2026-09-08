#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::joint_base_p55 {
class Agent:public joint_day_core::AgentCore {
public:
    Agent():AgentCore(55,true,true,true,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"joint_base_p55"};}
};
}
