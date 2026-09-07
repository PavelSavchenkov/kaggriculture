#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::leader_program0_tape {
class Agent {
public:
    static kag::agent::AgentInfo info() {return {"leader_program0_tape"};}
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
