#pragma once

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::cold_capacity_wheat {

class Agent {
public:
    static agent::AgentInfo info();
    void reset(const agent::AgentInit& init);
    void act(const agent::AgentObservation& observation,
             const agent::DecisionBudget& budget,
             Action& action);

private:
    agent::AgentConfig config_{};
    uint8_t player_ = 0;
};

static_assert(agent::LocalAgent<Agent>);

}
