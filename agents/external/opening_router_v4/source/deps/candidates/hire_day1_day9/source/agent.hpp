#pragma once
#include "../../hire_day1_counter/source/agent.hpp"
#include "../../../runs/reduce_hires_001/proposals/reduce_hires_001_15/source/agent.hpp"
namespace catalog_opening_router_v4_compositions::hire_day1_day9 {
inline std::vector<DayPlan> plans() {
    auto result=hire_day1_counter::plans();
    result.push_back({9,reduce_hires_001_15::schedule()});
    return result;
}
class Agent:public PlannedOpeningAgent {
public:
    Agent():PlannedOpeningAgent(plans()) {}
    static kag::agent::AgentInfo info() {return {"hire_day1_day9"};}
};
}
