#pragma once
#include "../../../../joint_day_routes_sep08_001/compiler/source/agent.hpp"
namespace compositions::cold_renewal_p1 {
#include "plan.inc"
class Agent:public joint_day_core::AgentCore {
public:
    Agent():AgentCore(std::vector<Life>(std::begin(lives),std::end(lives)),support,true,true,false,1,true,0,0,1) {}
    static kag::agent::AgentInfo info() {return {"cold_renewal_p1"};}
};
}
