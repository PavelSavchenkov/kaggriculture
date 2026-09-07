#include "agent.hpp"

#include <array>

namespace kag::agents::one_shop_no_geese_league_winner_v1 {
namespace {

void enforce_boundary_capacity(
    const kag::agent::AgentObservation& observation,
    kag::Action& action
) {
    if (observation.hour != 23) return;
    int projected = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int item = 0; item < kag::N_ITEMS; ++item)
            projected += observation.own.inv[unit][item];
    for (int order = 0; order < action.n_orders; ++order) {
        const kag::Order& value = action.orders[order];
        if (value.op == kag::M_SELL && value.item < kag::N_PRODUCTS)
            projected -= value.n;
        else if (value.op == kag::M_BUY_PRODUCT || value.op == kag::M_BUY_ANIMAL)
            projected += value.n;
    }
    struct Output {
        int unit = -1;
        int amount = 0;
        int value = 0;
    };
    std::array<Output, kag::MAX_UNITS> outputs{};
    int output_count = 0;
    for (int unit = 0; unit < action.n_units; ++unit) {
        const kag::UnitAction& selected = action.units[unit];
        if (selected.op == kag::OP_FEED || selected.op == kag::OP_FERTILIZE) {
            --projected;
            continue;
        }
        int item = -1;
        int amount = 0;
        if (selected.op == kag::OP_COLLECT_FERTILIZER) {
            item = kag::FERTILIZER;
            amount = 1;
        } else if (selected.op == kag::OP_HARVEST) {
            const int x = observation.self().pos_x[unit];
            const int y = observation.self().pos_y[unit];
            const kag::Tile& tile = observation.self().tiles[y][x];
            amount = tile.yield_units;
            item = tile.kind == kag::T_PLANT ? static_cast<int>(tile.what) :
                static_cast<int>(kag::ANIMALS[
                    tile.what - kag::GOOSE].product);
        }
        if (amount <= 0 || item < 0 || item >= kag::N_PRODUCTS) continue;
        projected += amount;
        outputs[output_count++] = {
            unit, amount, observation.market.prices[item] * amount};
    }
    while (projected > 100 && output_count > 0) {
        int cheapest = 0;
        for (int index = 1; index < output_count; ++index)
            if (outputs[index].value < outputs[cheapest].value)
                cheapest = index;
        action.units[outputs[cheapest].unit] = {};
        projected -= outputs[cheapest].amount;
        outputs[cheapest] = outputs[--output_count];
    }
    action.finalize();
}

}  // namespace

kag::agent::AgentInfo Agent::info() {
    return {"one_shop_no_geese_league_winner_v1"};
}

void Agent::reset(const kag::agent::AgentInit& init) {
    ::four_shop_search::compiled_portfolio_feed_early_sale_probe_v1::detail::RepairParameters repair;
    repair.cleanup_worker = false;
    repair.catch_up_sales = true;
    repair.liquidity_sales = true;
    repair.liquidity_opportunity_selector = true;
    repair.underfilled_sale_substitution = true;
    repair.early_pressure_sales = true;
    repair.terminal_closure = true;
    repair.terminal_drain_start_day = 29;
    repair.proactive_surplus_sales = true;
    repair.proactive_sell_inputs = true;
    repair.proactive_sale_start_day = 0;
    repair.proactive_sale_daily_units = 4;
    repair.demand_pulse_sales = true;
    repair.demand_pulse_units_per_item = 4;
    core_.set_parameters(repair);
    core_.reset(init);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    core_.act(observation, budget, action);
    enforce_boundary_capacity(observation, action);
}

}  // namespace kag::agents::one_shop_no_geese_league_winner_v1
