#pragma once
#include "day_timing.hpp"

namespace sales_planner {
// Sell currently held output earlier to make room for a more valuable delay.
// Price both changes together, through both original sale targets. Neither
// product may have a competing rival sale anywhere in the affected window.
inline Orders exchange_sale_holding(const PlannerObservation& obs,
                                     std::span<const CalendarTurn> calendar,
                                     std::span<Orders> plan, TimingMemory& memory,
                                     const MarketRules& rules,
                                     const std::array<int, kag::N_PRODUCTS>& rival_not_before,
                                     uint64_t max_turns) {
    if (rules.turns_per_day != 24 || calendar.size() != plan.size()) std::abort();
    const int last = std::min(int(plan.size()) - 1, (obs.turn / 24 + 1) * 24 - 1);
    const int length = last - obs.turn + 1;
    if (length < 2 || max_turns < uint64_t(length + 2)) return plan[obs.turn];
    const auto current = plan[obs.turn];
    auto unused = obs.own.stock;
    bool can_delay = false;
    for (int k = 0; k < current.count; ++k) {
        const auto order = current.values[k];
        if (order.op != kag::M_SELL || order.item >= kag::N_PRODUCTS || order.n <= 0) continue;
        unused[order.item] -= std::min(unused[order.item], order.n);
        can_delay |= order.item > kag::WHEAT && order.item < kag::FERTILIZER &&
            obs.own.stock[order.item] && rival_not_before[order.item] > obs.turn + 1;
    }
    if (!can_delay) return current;
    bool can_advance = false;
    for (int t = obs.turn + 1; t <= last; ++t)
        for (int k = 0; k < plan[t].count; ++k) {
            const auto order = plan[t].values[k];
            can_advance |= order.op == kag::M_SELL && order.item > kag::WHEAT &&
                order.item < kag::FERTILIZER && order.n > 0 && unused[order.item] > 0 &&
                rival_not_before[order.item] > t;
        }
    if (!can_advance) return current;

    DayProjection initial;
    initial.market.accounts[0] = obs.own; initial.market.accounts[1].cash = obs.rival_cash;
    initial.market.inventory = obs.inventory; initial.market.shops = obs.shops;
    initial.market.n_shops = obs.n_shops; initial.market.turn = obs.turn;
    initial.resources = obs.resources;
    auto before_market = [&](DayProjection& p, int turn) {
        if (turn > obs.turn) apply(p.market.accounts[0], p.resources, calendar[turn].before_market, rules.capacity);
    };
    auto after_market = [&](DayProjection& p, int turn) {
        consume(p.market, rules);
        apply(p.market.accounts[0], p.resources, calendar[turn].after_market, rules.capacity);
        advance(p.market, rules);
    };
    std::array<DayProjection, 24> reference;
    std::array<Orders, 24> original, best_plan;
    auto base = initial;
    uint64_t used = 0;
    for (int t = obs.turn; t <= last; ++t) {
        original[t - obs.turn] = plan[t];
        before_market(base, t); trade(base.market, {plan[t], Orders{}}, rules); ++used;
        reference[t - obs.turn] = base; after_market(base, t);
    }
    double best_gain = 0;
    int best_target = -1, best_item = -1, best_quantity = 0;
    for (int from = obs.turn + 1; from <= last; ++from) {
        for (int advance_slot = 0; advance_slot < plan[from].count; ++advance_slot) {
            const auto advance_order = plan[from].values[advance_slot];
            const int advanced = advance_order.item;
            if (advance_order.op != kag::M_SELL || advanced <= kag::WHEAT || advanced >= kag::FERTILIZER ||
                advance_order.n <= 0 || !unused[advanced] || rival_not_before[advanced] <= from) continue;
            const int advance_quantity = std::min(advance_order.n, unused[advanced]);
            for (int delay_slot = 0; delay_slot < current.count; ++delay_slot) {
                const auto delay_order = current.values[delay_slot];
                const int delayed = delay_order.item;
                if (delay_order.op != kag::M_SELL || delayed <= kag::WHEAT || delayed >= kag::FERTILIZER ||
                    delayed == advanced || delay_order.n <= 0) continue;
                const int delay_quantity = std::min(delay_order.n, obs.own.stock[delayed]);
                if (!delay_quantity) continue;
                for (int target = obs.turn + 1; target <= last && target < rival_not_before[delayed]; ++target) {
                    const int consumed = target - 1;
                    int demand = consumed % rules.town_interval == 0;
                    if (consumed % rules.shop_interval == 0)
                        for (int s = 0; s < obs.n_shops; ++s)
                            if (kag::SHOP_MASK[obs.shops[s]] & (1u << delayed)) demand += kag::SHOP_MULT[obs.shops[s]];
                    const int end = std::max(from, target);
                    if (end >= rival_not_before[advanced] || end >= rival_not_before[delayed]) continue;
                    if (!demand || used + end - obs.turn + 1 > max_turns) continue;
                    auto proposed = original;
                    proposed[0].values[delay_slot] = {};
                    auto& old_sale = proposed[from - obs.turn].values[advance_slot];
                    old_sale.n -= advance_quantity;
                    if (!old_sale.n) old_sale = {};
                    if (!add_sale(proposed[0], advanced, advance_quantity, rules.max_orders) ||
                        !add_sale(proposed[target - obs.turn], delayed, delay_quantity, rules.max_orders)) continue;
                    auto trial = initial;
                    bool valid = true;
                    for (int t = obs.turn; t <= end; ++t) {
                        before_market(trial, t); trade(trial.market, {proposed[t - obs.turn], Orders{}}, rules); ++used;
                        if (!same_work_resources(trial, reference[t - obs.turn], delayed, advanced)) { valid = false; break; }
                        if (t < end) after_market(trial, t);
                    }
                    const auto& expected = reference[end - obs.turn];
                    if (!valid || trial.market.accounts[0].stock != expected.market.accounts[0].stock ||
                        trial.market.inventory != expected.market.inventory) continue;
                    const double gain = trial.market.accounts[0].cash - expected.market.accounts[0].cash;
                    if (gain > best_gain) {
                        best_gain = gain; best_plan = proposed;
                        best_target = target; best_item = delayed; best_quantity = delay_quantity;
                    }
                }
            }
        }
    }
    memory.simulations += used;
    if (best_target >= 0) {
        std::copy_n(best_plan.begin(), length, plan.begin() + obs.turn);
        ++memory.decisions; ++memory.holding_exchanges;
        memory.delayed_units += best_quantity; memory.predicted_gain += best_gain;
        memory.last_delay_turn = obs.turn; memory.last_target = best_target;
        memory.last_item = best_item; memory.last_quantity = best_quantity;
    }
    return plan[obs.turn];
}

inline Orders improve_sale_holding(const PlannerObservation& obs,
                                    std::span<const CalendarTurn> calendar,
                                    std::span<Orders> plan,
                                    const std::array<int, kag::N_PRODUCTS>& rival_ready, uint16_t blocked,
                                    TimingMemory& memory, const MarketRules& rules,
                                    uint64_t max_turns,
                                    const std::array<int, kag::N_PRODUCTS>* rival_not_before) {
    if (!rival_not_before) std::abort();
    const auto initial_simulations = memory.simulations;
    delay_sales_within_day(obs, calendar, plan, rival_ready, blocked, memory, rules, max_turns, rival_not_before);
    const auto used = memory.simulations - initial_simulations;
    if (used < max_turns) {
        auto allowed = *rival_not_before;
        for (int item = 0; item < kag::N_PRODUCTS; ++item)
            if (blocked & (uint16_t{1} << item)) allowed[item] = obs.turn;
        exchange_sale_holding(obs, calendar, plan, memory, rules, allowed, max_turns - used);
    }
    return plan[obs.turn];
}
}
