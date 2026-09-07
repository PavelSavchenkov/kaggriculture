#pragma once

#include "agents/common/api/agent_api.hpp"
#include "base/agent.hpp"

namespace four_shop_foundry::public_adapted::deniz_v111_safe {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    kag::agent::AgentConfig config_{};
    four_shop_foundry::public_unchanged::deniz_v111::Agent source_;
};

}
