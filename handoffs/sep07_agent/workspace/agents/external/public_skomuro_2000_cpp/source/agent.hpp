#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace four_shop_search::public_skomuro_2000_cpp {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    int last_step_ = -1;
    int converted_ = 0;
    int last_wool_sell_ = -99;
    std::array<int16_t, kag::MAX_UNITS> delay_{};
    std::array<std::array<int16_t, kag::N_PRODUCTS>, 720> debt_{};
};

static_assert(kag::agent::LocalAgent<Agent>);

}  // namespace four_shop_search::public_skomuro_2000_cpp
