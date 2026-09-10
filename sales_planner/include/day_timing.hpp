#pragma once
#include "timing.hpp"

namespace sales_planner {
struct DayProjection {
    MarketState market;
    Resources resources;
};

inline bool same_work_resources(const DayProjection& a, const DayProjection& b, int delayed_item,
                                int advanced_item = -1) {
    const auto& x = a.market.accounts[0]; const auto& y = b.market.accounts[0];
    if (x.units != y.units || x.hires != y.hires || x.quadrants != y.quadrants || x.seeds != y.seeds ||
        a.resources.buffers != b.resources.buffers || a.resources.key_count != b.resources.key_count ||
        a.resources.missing != b.resources.missing || a.resources.discarded != b.resources.discarded) return false;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (item != delayed_item && item != advanced_item && x.stock[item] != y.stock[item]) return false;
    for (int u = 0; u < kag::MAX_UNITS; ++u)
        for (int k = 0; k < a.resources.key_count[u]; ++k)
            if (a.resources.keys[u][k] != b.resources.keys[u][k]) return false;
    return true;
}

// Move one currently planned output sale to a later known-demand event in the
// SAME day. The caller blocks possible rival private stock and supplies public
// currently harvestable yield. In the standard game, these goods
// cannot be bought or newly produced before the next night. This rules out
// rival sales of that product in the window, not rival trades of other goods.
// `plan` includes all previous commitments by this planner. Only a completely
// evaluated improvement mutates the current and selected future order lists.
inline Orders delay_within_day(const PlannerObservation& obs,
                               std::span<const CalendarTurn> calendar,
                               std::span<Orders> plan,
                               const std::array<int, kag::N_PRODUCTS>& rival_ready, uint16_t blocked,
                               TimingMemory& memory, const MarketRules& rules,
                               uint64_t max_turns = 1000,
                               const std::array<int, kag::N_PRODUCTS>* rival_not_before = nullptr) {
    if (rules.turns_per_day != 24 || calendar.size() != plan.size()) std::abort();
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        if (!rival_not_before && rival_ready[item]) blocked |= uint16_t{1} << item;
    const int last = std::min(int(plan.size()) - 1, (obs.turn / 24 + 1) * 24 - 1);
    const Orders current = plan[obs.turn];
    if (last <= obs.turn || max_turns < uint64_t(last - obs.turn + 2)) return current;
    bool eligible = false;
    for (int slot = 0; slot < current.count; ++slot) {
        const auto o = current.values[slot];
        eligible |= o.op == kag::M_SELL && o.item > kag::WHEAT && o.item < kag::FERTILIZER &&
            o.n > 0 && obs.own.stock[o.item] > 0 && !(blocked & (uint16_t{1} << o.item));
    }
    if (!eligible) return current;
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
    auto base = initial;
    uint64_t used = 0;
    for (int t = obs.turn; t <= last; ++t) {
        before_market(base, t); trade(base.market, {plan[t], Orders{}}, rules); ++used;
        reference[t - obs.turn] = base;
        after_market(base, t);
    }
    Orders best_now = current, best_later;
    int best_target = -1, best_quantity = 0, best_item = -1;
    double best_gain = 0;
    for (int slot = 0; slot < current.count; ++slot) {
        const auto order = current.values[slot];
        const int item = order.item;
        if (order.op != kag::M_SELL || item <= kag::WHEAT || item >= kag::FERTILIZER ||
            order.n <= 0 || (blocked & (uint16_t{1} << item))) continue;
        bool duplicate = false;
        for (int k = 0; k < current.count; ++k)
            if (k != slot && current.values[k].op == kag::M_SELL && current.values[k].item == item) duplicate = true;
        const int quantity = std::min(order.n, obs.own.stock[item]);
        if (!quantity || duplicate) continue;
        auto now = current; now.values[slot] = {};
        for (int target = obs.turn + 1; target <= last; ++target) {
            // An audited travel bound can refine the ready-yield exclusion.
            // Complete our sale strictly before any possible competing sale.
            if (rival_not_before && target >= (*rival_not_before)[item]) break;
            const int consumed = target - 1;
            int demand = consumed % rules.town_interval == 0;
            if (consumed % rules.shop_interval == 0)
                for (int s = 0; s < obs.n_shops; ++s)
                    if (kag::SHOP_MASK[obs.shops[s]] & (1u << item)) demand += kag::SHOP_MULT[obs.shops[s]];
            if (!demand || used + target - obs.turn + 1 > max_turns) continue;
            auto later = plan[target];
            if (!add_sale(later, item, quantity, rules.max_orders)) continue;
            auto trial = initial;
            bool valid = true;
            for (int t = obs.turn; t <= target; ++t) {
                before_market(trial, t);
                const auto& orders = t == obs.turn ? now : t == target ? later : plan[t];
                trade(trial.market, {orders, Orders{}}, rules); ++used;
                if (!same_work_resources(trial, reference[t - obs.turn], item)) { valid = false; break; }
                if (t < target) after_market(trial, t);
            }
            const auto& reference_end = reference[target - obs.turn];
            if (!valid || trial.market.accounts[0].stock != reference_end.market.accounts[0].stock ||
                trial.market.inventory != reference_end.market.inventory) continue;
            const double gain = trial.market.accounts[0].cash - reference_end.market.accounts[0].cash;
            if (gain > best_gain) {
                best_gain = gain; best_now = now; best_later = later;
                best_target = target; best_quantity = quantity; best_item = item;
            }
        }
    }
    memory.simulations += used;
    if (best_target >= 0) {
        plan[obs.turn] = best_now; plan[best_target] = best_later;
        ++memory.decisions; memory.delayed_units += best_quantity; memory.predicted_gain += best_gain;
        memory.last_delay_turn = obs.turn; memory.last_target = best_target;
        memory.last_item = best_item; memory.last_quantity = best_quantity;
    }
    return plan[obs.turn];
}

// Reuse any unspent budget for another current sale. Every accepted edit removes
// a current sale slot, so this ends without an additional edit-count threshold.
inline Orders delay_sales_within_day(const PlannerObservation& obs,
                                    std::span<const CalendarTurn> calendar,
                                    std::span<Orders> plan,
                                    const std::array<int, kag::N_PRODUCTS>& rival_ready, uint16_t blocked,
                                    TimingMemory& memory, const MarketRules& rules,
                                    uint64_t max_turns = 1000,
                                    const std::array<int, kag::N_PRODUCTS>* rival_not_before = nullptr) {
    const auto initial_simulations = memory.simulations;
    while (memory.simulations - initial_simulations < max_turns) {
        const auto before = memory.decisions;
        delay_within_day(obs, calendar, plan, rival_ready, blocked, memory, rules,
            max_turns - (memory.simulations - initial_simulations), rival_not_before);
        if (memory.decisions == before) break;
    }
    return plan[obs.turn];
}
}
