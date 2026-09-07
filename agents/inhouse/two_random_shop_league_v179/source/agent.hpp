#pragma once

#include "agents/common/api/agent_api.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/agent.hpp"

namespace kag::agents::two_random_shop_league_v179 {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    base::Agent c0_;
    base::Agent alternate_pet_;
    base::Agent alternate_target_;
    int held_units_ = 0;
    bool configured_ = false;
    bool initialized_ = false;

    void configure();
    void apply_market(const kag::agent::AgentObservation& observation,
                      kag::Action& action);
    void repair_transition_capacity(
        const kag::agent::AgentObservation& observation,
        kag::Action& action);
};

}  // namespace kag::agents::two_random_shop_league_v179
