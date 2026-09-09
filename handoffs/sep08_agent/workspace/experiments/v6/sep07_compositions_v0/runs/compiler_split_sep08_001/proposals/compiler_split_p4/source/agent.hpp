#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_split_p4 {
class Agent:public compiler_split_sep08::AgentCore {
public:
    Agent():AgentCore(4,true,true,true,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"compiler_split_p4"};}
};
}
