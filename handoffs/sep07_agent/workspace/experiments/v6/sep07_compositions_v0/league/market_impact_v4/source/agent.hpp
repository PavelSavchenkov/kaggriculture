#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::market_impact_v4 {
class Agent {
public:
    static kag::agent::AgentInfo info() { return {"market_impact_v4"}; }
    void reset(const kag::agent::AgentInit&) { route_ = 0; }
    const kag::Action& planned_action(int step) const;
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget&, kag::Action& action);
private:
    int route_ = 0;
};
}
