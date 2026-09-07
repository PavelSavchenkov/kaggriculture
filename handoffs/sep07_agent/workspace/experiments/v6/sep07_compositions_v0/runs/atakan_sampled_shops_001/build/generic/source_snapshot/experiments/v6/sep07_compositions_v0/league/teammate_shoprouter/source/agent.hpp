#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::teammate_shoprouter {
class Agent {
public:
    static kag::agent::AgentInfo info() { return {"teammate_shoprouter"}; }
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);
private:
    kag::Config config_{};
    int route_ = 0;
};
}
