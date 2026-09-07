#pragma once

#include "agents/common/api/agent_api.hpp"
#include "base/agent.hpp"

namespace four_shop_foundry::throughput::c68_h18_fast_sanitize {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    league::public_unchanged::c68::Policy source_{false};
    kag::agent::AgentConfig config_{};
};

}
