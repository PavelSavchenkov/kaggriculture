#include "agent.hpp"

#include <algorithm>

#include "optimized_active_safety.hpp"

namespace four_shop_foundry::throughput::boatlee_h7_fast_sanitize {
namespace {

int projected_capacity_load(kag::Sim sim, int player,
                            const kag::Action& action) {
    const kag::Action pass = four_shop_foundry::active_safety::pass_action(
        sim.st.farms[1 - player].n_units);
    if (player == 0) sim.step(action, pass);
    else sim.step(pass, action);
    const kag::Farm& farm = sim.st.farms[player];
    int total = farm.shed_total;
    for (int unit = 0; unit < farm.n_units; ++unit)
        for (int item = 0; item < kag::N_ITEMS; ++item)
            total += std::max<int>(0, farm.inv[unit][item]);
    for (int item = 0; item < kag::N_ITEMS; ++item)
        total += std::max(0, farm.discarded[item]);
    return total;
}

int immediate_yield(const kag::agent::AgentObservation& observation,
                    int unit, const kag::UnitAction& action) {
    const int x = observation.self().pos_x[unit];
    const int y = observation.self().pos_y[unit];
    const kag::Tile& tile = observation.self().tiles[y][x];
    if (action.op == kag::OP_HARVEST)
        return std::max<int>(0, tile.yield_units);
    if (action.op == kag::OP_COLLECT_FERTILIZER && tile.fertilizer_available)
        return 1;
    return 0;
}

long long capacity_upper_bound(
    const kag::agent::AgentObservation& observation,
    const kag::Action& action) {
    long long total = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int item = 0; item < kag::N_ITEMS; ++item)
            total += std::max<int>(0, observation.own.inv[unit][item]);
    for (int unit = 0; unit < action.n_units; ++unit)
        total += immediate_yield(observation, unit, action.units[unit]);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_BUY_PRODUCT || order.op == kag::M_BUY_ANIMAL)
            total += std::max(0, order.n);
    }
    return total;
}

void guard_total_capacity(const kag::Sim& original,
                          const kag::agent::AgentConfig& config,
                          const kag::agent::AgentObservation& observation,
                          kag::Action& action) {
    if (capacity_upper_bound(observation, action) <= config.shed_capacity)
        return;
    for (;;) {
        int overflow = projected_capacity_load(
            original, observation.player, action) - config.shed_capacity;
        if (overflow <= 0) return;
        bool changed = false;
        for (int index = action.n_orders - 1;
             index >= 0 && overflow > 0; --index) {
            kag::Order& order = action.orders[index];
            if ((order.op != kag::M_BUY_PRODUCT &&
                 order.op != kag::M_BUY_ANIMAL) || order.n <= 0)
                continue;
            const int reduction = std::min(overflow, std::max(0, order.n));
            order.n -= reduction;
            overflow -= reduction;
            changed = true;
        }
        for (int unit = action.n_units - 1;
             unit >= 0 && overflow > 0; --unit) {
            const int quantity = immediate_yield(
                observation, unit, action.units[unit]);
            if (quantity <= 0) continue;
            action.units[unit] = {};
            overflow -= quantity;
            changed = true;
        }
        if (!changed) return;
        action.finalize();
        four_shop_foundry::optimized_active_safety::sanitize(
            original, config, observation, action);
    }
}

}

kag::agent::AgentInfo Agent::info() {
    return {"boatlee-h7-fast-sanitize"};
}

void Agent::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    source_.reset(init);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    source_.act(observation, budget, action);
    const kag::Sim sim = four_shop_foundry::observation_sim::make_sim(
        config_, observation);
    four_shop_foundry::optimized_active_safety::sanitize(
        sim, config_, observation, action);
    guard_total_capacity(sim, config_, observation, action);
}

}
