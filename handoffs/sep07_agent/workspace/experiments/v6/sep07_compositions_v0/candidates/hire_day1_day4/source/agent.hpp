#pragma once
#include "../../hire_day1_counter/source/agent.hpp"
#include "../../../runs/reduce_hires_001/proposals/reduce_hires_001_25/source/agent.hpp"
namespace compositions::hire_day1_day4 {
inline std::vector<DayPlan> plans() {
    auto result=hire_day1_counter::plans();
    result.push_back({4,reduce_hires_001_25::schedule()});
    return result;
}
class Agent:public PlannedOpeningAgent {
public:
    Agent():PlannedOpeningAgent(plans()) {}
    static kag::agent::AgentInfo info() {return {"hire_day1_day4"};}
};
}
