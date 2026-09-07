#include "raw_agent.hpp"

#include <algorithm>
#include <cstdlib>

namespace four_shop_search::external_replay_band_adamjonesjohnson_89413383 {

#include "tape.inc"

kag::agent::AgentInfo Agent::info() { return {"external_replay_band_adamjonesjohnson_89413383"}; }

void Agent::reset(const kag::agent::AgentInit& init) { player_ = init.player; }

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    if (observation.player != player_) std::abort();
    action.clear();
    action.n_units = 1 + observation.own_hand_count();
    for (int unit = 0; unit < action.n_units; ++unit) action.units[unit] = {};
    if (observation.step < 0 || observation.step >= TAPE_STEPS) {
        action.finalize();
        return;
    }
    int cursor = TAPE_OFFSETS[observation.step];
    const int tape_units = TAPE_DATA[cursor++];
    const int tape_orders = TAPE_DATA[cursor++];
    for (int unit = 0; unit < tape_units; ++unit) {
        const kag::UnitAction value{
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2],
        };
        cursor += 3;
        if (unit < action.n_units) action.units[unit] = value;
    }
    action.n_orders = std::min(tape_orders, 10);
    for (int order = 0; order < tape_orders; ++order) {
        const kag::Order value{
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2],
        };
        cursor += 3;
        if (order < action.n_orders) action.orders[order] = value;
    }
    action.finalize();
}

}
