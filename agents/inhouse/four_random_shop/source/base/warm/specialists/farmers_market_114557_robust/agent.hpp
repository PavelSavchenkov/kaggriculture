#pragma once

#include <memory>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::four_random_shop::base::warm::farmers_market_114557_robust {

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
