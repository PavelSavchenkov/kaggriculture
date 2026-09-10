#pragma once
#include "day_timing.hpp"

namespace sales_planner {
struct SaleSlotStats {
    uint64_t simulations = 0, sale_advances = 0, advanced_units = 0;
    uint64_t seed_prebuys = 0, seed_units = 0;
    double predicted_gain = 0;
};

inline int empty_order_slot(const Orders& orders, int limit) {
    for (int k = 0; k < limit; ++k)
        if (k >= orders.count || orders.values[k].op == kag::M_NONE) return k;
    return -1;
}

inline bool preserves_two_turn_plan(const TimingProjection& a, const TimingProjection& b) {
    return a.current_commitments == b.current_commitments && a.state.inventory == b.state.inventory &&
        same_work_resources({a.state, a.resources}, {b.state, b.resources}, -1);
}

// Fill a hole in a crowded order list with an already scheduled next-turn
// output sale. Use public competition risk, not the rival's future orders.
inline void fill_sale_slot(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                           std::span<Orders> plan, const std::array<int, kag::N_PRODUCTS>& rival_not_before,
                           SaleSlotStats& stats, const MarketRules& rules, uint64_t max_turns) {
    if (obs.turn + 1 >= int(plan.size()) || max_turns < 4 ||
        plan[obs.turn].count != rules.max_orders || empty_order_slot(plan[obs.turn], rules.max_orders) < 0) return;
    const auto now = plan[obs.turn], next = plan[obs.turn + 1];
    bool eligible = false;
    for (int k = 0; k < next.count; ++k) {
        const auto order = next.values[k];
        eligible |= order.op == kag::M_SELL && order.item > kag::WHEAT && order.item < kag::FERTILIZER &&
            order.n > 0 && obs.own.stock[order.item] > 0 && rival_not_before[order.item] <= obs.turn + 1;
    }
    if (!eligible) return;
    auto reference = project_two_turns(obs, calendar, now, next, rules);
    uint64_t used = 2;
    int best_quantity = 0, best_score = 0;
    double best_gain = 0;
    Orders best_now, best_next;
    for (int k = 0; k < next.count && used + 2 <= max_turns; ++k) {
        const auto order = next.values[k]; const int item = order.item;
        if (order.op != kag::M_SELL || item <= kag::WHEAT || item >= kag::FERTILIZER ||
            order.n <= 0 || rival_not_before[item] > obs.turn + 1) continue;
        bool already_sold = false;
        for (int j = 0; j < now.count; ++j)
            already_sold |= now.values[j].op == kag::M_SELL && now.values[j].item == item && now.values[j].n > 0;
        const int quantity = std::min(order.n, obs.own.stock[item]);
        const int score = quantity * kag::market_price(item, obs.inventory[item]);
        if (already_sold || !quantity || score <= best_score) continue;
        auto proposed_now = now, proposed_next = next;
        if (!add_sale(proposed_now, item, quantity, rules.max_orders)) continue;
        proposed_next.values[k].n -= quantity;
        if (!proposed_next.values[k].n) proposed_next.values[k] = {};
        const auto trial = project_two_turns(obs, calendar, proposed_now, proposed_next, rules); used += 2;
        if (!preserves_two_turn_plan(trial, reference)) continue;
        best_now = proposed_now; best_next = proposed_next; best_score = score; best_quantity = quantity;
        best_gain = trial.state.accounts[0].cash - reference.state.accounts[0].cash;
    }
    stats.simulations += used;
    if (best_quantity) {
        plan[obs.turn] = best_now; plan[obs.turn + 1] = best_next;
        ++stats.sale_advances; stats.advanced_units += best_quantity; stats.predicted_gain += best_gain;
    }
}

// Move one supplied seed order one turn earlier to free a crowded future slot.
// Seeds have fixed prices and separate storage. Check full funding and both
// turns' resource effects; preserve the complete quantity and all other orders.
inline void prebuy_seed_order(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                              std::span<Orders> plan, SaleSlotStats& stats,
                              const MarketRules& rules, uint64_t max_turns) {
    if (obs.turn + 2 >= int(plan.size()) || max_turns < 6) return;
    const auto now = plan[obs.turn], next = plan[obs.turn + 1];
    const int slot = empty_order_slot(now, rules.max_orders);
    if (slot < 0 || empty_order_slot(next, rules.max_orders) >= 0) return;
    bool has_seed = false;
    for (int k = 0; k < next.count; ++k) has_seed |= next.values[k].op == kag::M_BUY_SEED && next.values[k].n > 0;
    if (!has_seed) return;
    MarketState initial;
    initial.accounts[0] = obs.own; initial.accounts[1].cash = obs.rival_cash;
    initial.inventory = obs.inventory; initial.shops = obs.shops;
    initial.n_shops = obs.n_shops; initial.turn = obs.turn;
    auto arrival = initial; auto resources = obs.resources;
    trade(arrival, {now, Orders{}}, rules); consume(arrival, rules);
    apply(arrival.accounts[0], resources, calendar[obs.turn].after_market, rules.capacity);
    advance(arrival, rules);
    apply(arrival.accounts[0], resources, calendar[obs.turn + 1].before_market, rules.capacity);
    ++stats.simulations;
    bool sale_waiting = false;
    for (int k = 0; k < plan[obs.turn + 2].count; ++k) {
        const auto order = plan[obs.turn + 2].values[k];
        if (order.op != kag::M_SELL || order.item <= kag::WHEAT || order.item >= kag::FERTILIZER ||
            order.n <= 0 || !arrival.accounts[0].stock[order.item]) continue;
        bool sold_next = false;
        for (int j = 0; j < next.count; ++j)
            sold_next |= next.values[j].op == kag::M_SELL && next.values[j].item == order.item && next.values[j].n > 0;
        sale_waiting |= !sold_next;
    }
    if (!sale_waiting) return;
    const auto reference = project_two_turns(obs, calendar, now, next, rules);
    uint64_t used = 3;
    int best_quantity = 0, best_cost = std::numeric_limits<int>::max();
    Orders best_now, best_next;
    for (int k = 0; k < next.count && used + 3 <= max_turns; ++k) {
        const auto order = next.values[k];
        if (order.op != kag::M_BUY_SEED || order.n <= 0 || !kag::is_crop(order.item)) continue;
        const int cost = kag::CROPS[order.item].seed * order.n;
        if (cost >= best_cost) continue;
        auto proposed_now = now, proposed_next = next;
        proposed_now.values[slot] = order; proposed_now.count = std::max(proposed_now.count, slot + 1);
        proposed_next.values[k] = {};
        auto funded = initial; const auto executed = trade(funded, {proposed_now, Orders{}}, rules); ++used;
        if (executed.accepted[0][slot] != order.n) continue;
        const auto trial = project_two_turns(obs, calendar, proposed_now, proposed_next, rules); used += 2;
        if (!preserves_two_turn_plan(trial, reference) || trial.state.accounts[0].cash != reference.state.accounts[0].cash) continue;
        best_now = proposed_now; best_next = proposed_next; best_quantity = order.n; best_cost = cost;
    }
    stats.simulations += used - 1; // The arrival projection was already counted.
    if (best_quantity) {
        plan[obs.turn] = best_now; plan[obs.turn + 1] = best_next;
        ++stats.seed_prebuys; stats.seed_units += best_quantity;
    }
}
}
