#pragma once
#include "../../../days.hpp"
#include "../../../../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
namespace compositions::dated_days_control {
class Agent:public GuardedDayAgent<dated_expansion_p362::Agent> {
public:
    Agent():GuardedDayAgent<dated_expansion_p362::Agent>(std::vector<GuardedDay>{}) {}
    static kag::agent::AgentInfo info() {return {"dated_days_control"};}
};
}
