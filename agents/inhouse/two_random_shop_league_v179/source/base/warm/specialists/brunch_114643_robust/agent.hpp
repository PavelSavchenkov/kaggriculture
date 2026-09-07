#pragma once

#include <memory>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::brunch_114643_robust {

class Agent {
public:
    Agent();
    ~Agent();

    Agent(const Agent&) = delete;
    Agent& operator=(const Agent&) = delete;
    Agent(Agent&&) noexcept;
    Agent& operator=(Agent&&) noexcept;

    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(
        const kag::agent::AgentObservation& observation,
        const kag::agent::DecisionBudget& budget,
        kag::Action& action
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
