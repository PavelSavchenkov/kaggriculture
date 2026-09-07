#pragma once

#include "agents/common/api/agent_api.hpp"

namespace four_shop_foundry::replay::replay_arman_ge3000_94541153 {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    uint8_t player_ = 0;
};

}
