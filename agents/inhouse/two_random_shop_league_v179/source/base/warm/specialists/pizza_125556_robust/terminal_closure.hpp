#pragma once

#include <algorithm>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza {

// Observation-only terminal exchange. Step 716 is the last turn on which the
// inherited route's milk carrier is idle at shed access. Closing milk then
// preserves the incumbent egg and fertilizer deposits on steps 717 and 718.
inline int close_terminal_milk(
    const kag::agent::AgentObservation& observation,
    kag::Action& action
) {
    if (observation.step < 716) return 0;
    int unit = -1;
    int amount = 0;
    for (int candidate = 0; candidate < observation.self().n_units;
         ++candidate) {
        const int milk = observation.own.inv[candidate][kag::MILK];
        if (milk <= amount || !kag::is_shed_adjacent(
                observation.self().pos_x[candidate],
                observation.self().pos_y[candidate], kag::BOARD))
            continue;
        unit = candidate;
        amount = milk;
    }
    if (unit < 0 || action.n_orders >= 10) return 0;
    action.units[unit] = {kag::OP_PLACE, kag::MILK, amount};
    action.orders[action.n_orders++] = {kag::M_SELL, kag::MILK, amount};
    action.finalize();
    return amount;
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza
