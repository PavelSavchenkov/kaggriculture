#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::fresh_courses_1642 {
int program_count();
class Agent {
public:
    explicit Agent(int program=0);
    static kag::agent::AgentInfo info() {return {"fresh_courses_1642"};}
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
private:
    int program_=0;
};
}
