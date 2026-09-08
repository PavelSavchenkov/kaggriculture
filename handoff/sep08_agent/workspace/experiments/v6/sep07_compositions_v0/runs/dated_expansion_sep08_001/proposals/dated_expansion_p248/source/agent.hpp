#pragma once
#include "../../../../compiler_care_sep08_001/compiler/source/agent.hpp"
namespace compositions::dated_expansion_p248 {
#include "plan.inc"
class Agent:public compiler_care_sep08::AgentCore {
public:
    Agent():AgentCore(std::vector<Life>(std::begin(lives),std::end(lives)),support,true,true,false,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"dated_expansion_p248"};}
};
}
