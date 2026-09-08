#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
#include "../../../../dated_expansion_sep08_001/proposals/dated_expansion_p355/source/agent.hpp"
namespace compositions::joint_base_p355 {
class Agent:public joint_day_core::AgentCore {
public:
    Agent():AgentCore(std::vector<Life>(std::begin(dated_expansion_p355::lives),std::end(dated_expansion_p355::lives)),dated_expansion_p355::support,true,true,false,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"joint_base_p355"};}
};
}
