#pragma once

#include "agents/common/api/agent_api.hpp"

namespace four_shop_search::external_replay_band_xdang13_89917554 {

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
