#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "observation_sim.hpp"

namespace league::public_optimized::kaito_h10_front_run_fast {
namespace {

int inventory_total(const kag::agent::AgentObservation& observation,
                    int unit) {
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        total += std::max<int>(0, observation.own.inv[unit][item]);
    return total;
}

void reject_overflowing_drops(
    int shed_capacity,
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    int headroom = std::max(0, shed_capacity - observation.own.shed_total);
    for (int unit = 0; unit < action.n_units; ++unit) {
        if (action.units[unit].op != kag::OP_DROP) continue;
        const int amount = inventory_total(observation, unit);
        if (amount > headroom) action.units[unit] = {};
        else headroom -= amount;
    }
    action.finalize();
}

kag::Action pass_action(int units) {
    kag::Action action;
    action.clear();
    action.n_units = units;
    for (int unit = 0; unit < units; ++unit) action.units[unit] = {};
    action.finalize();
    return action;
}

kag::Action exact_fast_sanitize(const kag::Sim& sim, int player,
                                const kag::Action& requested,
                                SanitizerCounters& counters) {
    if (requested.n_units == sim.st.farms[player].n_units &&
        requested.n_orders >= 0 && requested.n_orders <= sim.cfg.max_orders) {
        const auto outcome = sim.diagnose_solo_action(
            player, requested, true);
        if (outcome.requested_unit_actions == outcome.successful_unit_actions &&
            outcome.requested_order_units == outcome.successful_order_units) {
            ++counters.fast_path;
            kag::Action result = requested;
            for (int unit = 0; unit < result.n_units; ++unit)
                if (result.units[unit].op == kag::OP_PASS)
                    result.units[unit] = {};
            for (int index = 0; index < result.n_orders; ++index)
                if (result.orders[index].op == kag::M_NONE ||
                    (result.orders[index].op != kag::M_HIRE &&
                     result.orders[index].op != kag::M_BUY_LAND &&
                     result.orders[index].n <= 0))
                    result.orders[index] = {};
            result.finalize();
            return result;
        }
    }
    ++counters.fallback;
    return sim.sanitize_solo_action(player, requested);
}

int candidate_order_failures(const kag::Sim& sim, int player,
                             const kag::Action& candidate,
                             const kag::Action& opponent) {
    const auto result = player == 0 ?
        sim.diagnose_joint_actions(candidate, opponent) :
        sim.diagnose_joint_actions(opponent, candidate);
    const auto& own = result.players[player];
    return own.requested_order_units - own.successful_order_units;
}

bool opponent_is_active(const kag::agent::AgentObservation& observation) {
    const auto& farm = observation.opponent();
    if (farm.n_units > 1 || farm.n_quadrants > 1) return true;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            if (farm.tiles[y][x].kind != kag::T_EMPTY &&
                farm.tiles[y][x].kind != kag::T_WEED)
                return true;
    return false;
}

void guard_variable_price_buys(kag::Sim sim, int player,
                               kag::Action& action) {
    constexpr int TARGET_MAX_BUY = 18;
    sim.st.farms[1 - player].money = 1e9;
    sim.st.farms[1 - player].shed_total = 0;
    constexpr std::array<int, 2> ITEMS{kag::WHEAT, kag::FERTILIZER};
    for (const int item : ITEMS) {
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
            int reduced = -1;
            for (int index = action.n_orders - 1; index >= 0; --index) {
                kag::Order& order = action.orders[index];
                if (order.op != kag::M_BUY_PRODUCT || order.item != item ||
                    order.n <= 0)
                    continue;
                --order.n;
                reduced = index;
                break;
            }
            if (reduced < 0) std::abort();
        }
    }
    action.finalize();
}

int discards_after_step(kag::Sim sim, int player,
                        const kag::Action& candidate) {
    const kag::Action pass = pass_action(sim.st.farms[1 - player].n_units);
    if (player == 0) sim.step(candidate, pass);
    else sim.step(pass, candidate);
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        total += sim.st.farms[player].discarded[item];
    return total;
}

void liquidate_for_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    action.n_orders = 0;
    std::array<int, kag::N_PRODUCTS> available{};
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

void safety_shell(const kag::agent::AgentConfig& config,
                  const kag::Sim& sim,
                  bool active_opponent,
                  int shed_capacity,
                  const kag::agent::AgentObservation& observation,
                  kag::Action& action, SanitizerCounters& counters) {
    reject_overflowing_drops(shed_capacity, observation, action);
    action = exact_fast_sanitize(sim, observation.player, action, counters);
    if (active_opponent)
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
    action = exact_fast_sanitize(sim, observation.player, action, counters);
    if (active_opponent)
        guard_variable_price_buys(sim, observation.player, action);
    if (discards_after_step(sim, observation.player, action) == 0) {
        action.finalize();
        return;
    }
    for (int unit = 0; unit < action.n_units; ++unit) action.units[unit] = {};
    liquidate_for_capacity(observation, action);
    action = exact_fast_sanitize(sim, observation.player, action, counters);
    if (active_opponent)
        guard_variable_price_buys(sim, observation.player, action);
    action.finalize();
}

void insert_step_zero_slot(kag::Action& action) {
    constexpr int SLOT = 5;
    if (action.n_orders != 9) std::abort();
    for (int index = action.n_orders; index > SLOT; --index)
        action.orders[index] = action.orders[index - 1];
    ++action.n_orders;
    action.orders[SLOT] = {kag::M_BUY_PRODUCT, kag::WHEAT, 2};
    action.finalize();
}

void remove_step_one_buy(kag::Action& action) {
    int found = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        kag::Order& order = action.orders[index];
        if (order.op != kag::M_BUY_PRODUCT || order.item != kag::WHEAT ||
            order.n != 2)
            continue;
        order = {};
        ++found;
    }
    if (found != 1) std::abort();
    action.finalize();
}

}

kag::agent::AgentInfo Policy::info() {
    return {"public-kaito-h10-front-run-fast"};
}

void Policy::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    counters_ = {};
    source_.reset(init);
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget& budget,
                 kag::Action& action) {
    source_.act(observation, budget, action);
    const kag::Sim sim = four_shop_foundry::observation_sim::make_sim(
        config_, observation);
    const bool active_opponent = opponent_is_active(observation);
    if (observation.step == 0 || observation.step == 1) {
        safety_shell(config_, sim, active_opponent, 100,
                     observation, action, counters_);
    }
    if (observation.step == 0)
        insert_step_zero_slot(action);
    else if (observation.step == 1)
        remove_step_one_buy(action);
    safety_shell(config_, sim, active_opponent, config_.shed_capacity,
                 observation, action, counters_);
}

}
