#pragma once

#include <algorithm>

#include "observation_sim.hpp"

namespace four_shop_foundry::active_safety {

inline int inventory_total(const kag::agent::AgentObservation& observation,
                           int unit) {
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        total += std::max<int>(0, observation.own.inv[unit][item]);
    return total;
}

inline void reject_overflowing_drops(
    const kag::agent::AgentConfig& config,
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    int headroom = std::max(0, config.shed_capacity - observation.own.shed_total);
    for (int unit = 0; unit < action.n_units; ++unit) {
        if (action.units[unit].op != kag::OP_DROP) continue;
        const int amount = inventory_total(observation, unit);
        if (amount > headroom) action.units[unit] = {};
        else headroom -= amount;
    }
    action.finalize();
}

inline kag::Action pass_action(int units) {
    kag::Action action;
    action.clear();
    action.n_units = units;
    for (int unit = 0; unit < units; ++unit) action.units[unit] = {};
    action.finalize();
    return action;
}

inline int candidate_order_failures(const kag::Sim& sim, int player,
                                    const kag::Action& candidate,
                                    const kag::Action& opponent) {
    const auto result = player == 0 ?
        sim.diagnose_joint_actions(candidate, opponent) :
        sim.diagnose_joint_actions(opponent, candidate);
    const auto& own = result.players[player];
    return own.requested_order_units - own.successful_order_units;
}

inline bool opponent_is_active(
    const kag::agent::AgentObservation& observation) {
    const auto& farm = observation.opponent();
    if (farm.n_units > 1 || farm.n_quadrants > 1) return true;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            if (farm.tiles[y][x].kind != kag::T_EMPTY &&
                farm.tiles[y][x].kind != kag::T_WEED)
                return true;
    return false;
}

inline void guard_variable_price_buys(kag::Sim sim, int player,
                                      kag::Action& action) {
    constexpr int TARGET_MAX_BUY = 18;
    sim.st.farms[1 - player].money = 1e9;
    sim.st.farms[1 - player].shed_total = 0;
    for (int item : {kag::WHEAT, kag::FERTILIZER}) {
        bool has_buy = false;
        for (int index = 0; index < action.n_orders; ++index)
            has_buy = has_buy ||
                (action.orders[index].op == kag::M_BUY_PRODUCT &&
                 action.orders[index].item == item &&
                 action.orders[index].n > 0);
        if (!has_buy) continue;
        kag::Action hostile = pass_action(sim.st.farms[1 - player].n_units);
        hostile.n_orders = action.n_orders;
        for (int index = 0; index < hostile.n_orders; ++index)
            hostile.orders[index] = {
                kag::M_BUY_PRODUCT, static_cast<uint8_t>(item),
                TARGET_MAX_BUY};
        hostile.finalize();
        while (candidate_order_failures(sim, player, action, hostile) > 0) {
            bool reduced = false;
            for (int index = action.n_orders - 1; index >= 0; --index) {
                kag::Order& order = action.orders[index];
                if (order.op != kag::M_BUY_PRODUCT || order.item != item ||
                    order.n <= 0)
                    continue;
                --order.n;
                reduced = true;
                break;
            }
            if (!reduced) std::abort();
        }
    }
    action.finalize();
}

inline int discards_after_step(kag::Sim sim, int player,
                               const kag::Action& candidate) {
    const kag::Action pass = pass_action(sim.st.farms[1 - player].n_units);
    if (player == 0) sim.step(candidate, pass);
    else sim.step(pass, candidate);
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        total += sim.st.farms[player].discarded[item];
    return total;
}

inline void liquidate_for_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    action.n_orders = 0;
    int available[kag::N_PRODUCTS]{};
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        available[item] = std::max<int>(0, observation.own.shed[item]);
    for (int unit = 0; unit < action.n_units; ++unit) {
        if (action.units[unit].op != kag::OP_DROP) continue;
        for (int item = 0; item < kag::N_PRODUCTS; ++item)
            available[item] += std::max<int>(
                0, observation.own.inv[unit][item]);
    }
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        if (available[item] <= 0) continue;
        action.orders[action.n_orders++] = {
            kag::M_SELL, static_cast<uint8_t>(item), available[item]};
    }
    action.finalize();
}

inline void sanitize(const kag::agent::AgentConfig& config,
                     const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    reject_overflowing_drops(config, observation, action);
    kag::Sim sim = four_shop_foundry::observation_sim::make_sim(
        config, observation);
    action = sim.sanitize_solo_action(observation.player, action);
    if (opponent_is_active(observation))
        guard_variable_price_buys(sim, observation.player, action);
    if (observation.hour + 1 != config.turns_per_day) {
        action.finalize();
        return;
    }
    if (discards_after_step(sim, observation.player, action) == 0) {
        action.finalize();
        return;
    }
    liquidate_for_capacity(observation, action);
    action = sim.sanitize_solo_action(observation.player, action);
    if (opponent_is_active(observation))
        guard_variable_price_buys(sim, observation.player, action);
    if (discards_after_step(sim, observation.player, action) == 0) {
        action.finalize();
        return;
    }
    for (int unit = 0; unit < action.n_units; ++unit) action.units[unit] = {};
    liquidate_for_capacity(observation, action);
    action = sim.sanitize_solo_action(observation.player, action);
    if (opponent_is_active(observation))
        guard_variable_price_buys(sim, observation.player, action);
    action.finalize();
}

}
