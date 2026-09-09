#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_care_p4_productive_m0 {
class Agent:public compiler_care_sep08::AgentCore {
public:
    Agent():AgentCore(4,true,true,false,1,true,0,0,0) {}
    static kag::agent::AgentInfo info() {return {"compiler_care_p4_productive_m0"};}
};
}
