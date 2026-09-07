#pragma once

#include "agents/common/api/agent_api.hpp"

namespace four_shop_foundry::replay::test_replay_mrkiwi_ge3000_93167917 {

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
