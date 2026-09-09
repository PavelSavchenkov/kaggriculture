#pragma once
#include "../../../days.hpp"
#include "../../../../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
namespace compositions::dated_days_labor {
class Agent:public GuardedDayAgent<dated_expansion_p362::Agent> {
public:
    Agent():GuardedDayAgent<dated_expansion_p362::Agent>(std::vector<GuardedDay>{dated_days_data::labor_14(),dated_days_data::labor_18(),dated_days_data::labor_22(),dated_days_data::labor_26()}) {}
    static kag::agent::AgentInfo info() {return {"dated_days_labor"};}
};
}
