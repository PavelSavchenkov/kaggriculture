#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_care_p55_productive_m1 {
class Agent:public compiler_care_sep08::AgentCore {
public:
    Agent():AgentCore(55,true,true,false,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"compiler_care_p55_productive_m1"};}
};
}
