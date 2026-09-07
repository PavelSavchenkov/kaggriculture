#pragma once

#include "agents/common/api/agent_api.hpp"
#include "core.hpp"

namespace kag::agents::one_shop_no_geese_league_winner_v1 {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget,
             kag::Action& action);

private:
    ::four_shop_search::compiled_portfolio_feed_early_sale_probe_v1::detail::CoreAgent core_{};
};

static_assert(kag::agent::LocalAgent<Agent>);

}  // namespace kag::agents::one_shop_no_geese_league_winner_v1
