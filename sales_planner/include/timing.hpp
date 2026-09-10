#pragma once
#include "baseline.hpp"

namespace sales_planner {
struct TimingMemory {
    Items pending{};
    std::array<int, kag::N_PRODUCTS> last_ready{};
    uint64_t simulations = 0, delayed_units = 0, decisions = 0;
    uint64_t holding_exchanges = 0;
    double predicted_gain = 0;
    int last_delay_turn = -1, last_target = -1, last_item = -1, last_quantity = 0;
    TimingMemory() { last_ready.fill(-100); }
};

inline bool add_sale(Orders& orders, int item, int quantity, int max_orders) {
    if (quantity <= 0) return true;
    for (int slot = 0; slot < orders.count; ++slot)
        if (orders.values[slot].op == kag::M_SELL && orders.values[slot].item == item) {
            orders.values[slot].n += quantity;
            return true;
        }
    for (int slot = 0; slot < max_orders; ++slot)
        if (slot >= orders.count || orders.values[slot].op == kag::M_NONE) {
            orders.values[slot] = {kag::M_SELL, uint8_t(item), quantity};
            orders.count = std::max(orders.count, slot + 1);
            return true;
        }
    return false;
}

struct TimingProjection {
    MarketState state;
    Resources resources;
    std::array<int, 3> current_commitments{};
};

inline TimingProjection project_two_turns(const PlannerObservation& obs,
                                         std::span<const CalendarTurn> calendar,
                                         const Orders& now, const Orders& next,
                                         const MarketRules& rules) {
    TimingProjection out;
    out.state.accounts[0] = obs.own; out.state.accounts[1].cash = obs.rival_cash;
    out.state.inventory = obs.inventory; out.state.turn = obs.turn;
    out.state.shops = obs.shops; out.state.n_shops = obs.n_shops;
    out.resources = obs.resources;
    trade(out.state, {now, Orders{}}, rules);
    const auto& current = out.state.accounts[0];
    out.current_commitments = {current.units, current.hires, current.quadrants};
    consume(out.state, rules);
    apply(out.state.accounts[0], out.resources, calendar[obs.turn].after_market, rules.capacity);
    advance(out.state, rules);
    apply(out.state.accounts[0], out.resources, calendar[obs.turn + 1].before_market, rules.capacity);
    trade(out.state, {next, Orders{}}, rules);
    return out;
}

// A deliberately simple warm-start rule: delay one sale across a known
// consumption event. It assumes no rival sale in the two-turn projection;
// current/recent public ready yield is an optional guard, not hidden stock.
// The caller evaluates the rule against hidden rival actions and future worlds.
inline Orders delay_for_demand(const PlannerObservation& obs,
                               std::span<const CalendarTurn> calendar,
                               const Orders& warm_now, const Orders& warm_next,
                               const std::array<int, kag::N_PRODUCTS>& rival_ready,
                               TimingMemory& memory, const MarketRules& rules,
                               bool check_rival = true, uint64_t max_simulations = 20,
                               bool allow_mixed = false, uint16_t blocked_products = 0,
                               bool current_ready_only = false, bool preserve_market_inventory = false) {
    Orders current = warm_now;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        if (rival_ready[item]) memory.last_ready[item] = obs.turn;
        if (memory.pending[item]) {
            if (!add_sale(current, item, memory.pending[item], rules.max_orders)) std::abort();
            memory.pending[item] = 0;
        }
    }
    if (obs.turn + 1 >= int(calendar.size()) || obs.turn % 4 != 0 || max_simulations < 2) return current;
    if (!allow_mixed)
        for (int slot = 0; slot < current.count; ++slot)
            if (current.values[slot].op != kag::M_NONE && current.values[slot].op != kag::M_SELL) return current;
    const auto reference = project_two_turns(obs, calendar, current, warm_next, rules);
    ++memory.simulations;
    Orders best = current;
    double best_gain = 0;
    int best_item = -1, best_quantity = 0;
    uint64_t local = 1;
    for (int slot = 0; slot < current.count && local < max_simulations; ++slot) {
        const auto order = current.values[slot];
        if (order.op != kag::M_SELL || order.item >= kag::N_PRODUCTS || order.n <= 0) continue;
        const int item = order.item;
        if (blocked_products & (uint16_t{1} << item)) continue;
        if (check_rival && obs.turn - memory.last_ready[item] <= (current_ready_only ? 0 : 4)) continue;
        int absorption = item != kag::FERTILIZER && obs.turn % rules.town_interval == 0;
        if (obs.turn % rules.shop_interval == 0)
            for (int i = 0; i < obs.n_shops; ++i)
                if (kag::SHOP_MASK[obs.shops[i]] & (1u << item)) absorption += kag::SHOP_MULT[obs.shops[i]];
        if (!absorption) continue;
        bool duplicate = false;
        for (int j = 0; j < current.count; ++j)
            if (j != slot && current.values[j].op == kag::M_SELL && current.values[j].item == item) duplicate = true;
        if (duplicate) continue;
        const int n = std::min(order.n, obs.own.stock[item]);
        if (!n) continue;
        auto trial = current, next = warm_next;
        trial.values[slot] = {};
        if (!add_sale(next, item, n, rules.max_orders)) continue;
        const auto projected = project_two_turns(obs, calendar, trial, next, rules);
        ++local; ++memory.simulations;
        // Floor-price sales add no inventory. Equal ending private stock can
        // therefore hide a changed market and worse prices on later sales.
        if (preserve_market_inventory && projected.state.inventory != reference.state.inventory) continue;
        const auto& a = projected.state.accounts[0]; const auto& b = reference.state.accounts[0];
        if (allow_mixed && projected.current_commitments != reference.current_commitments) continue;
        if (a.stock != b.stock || a.seeds != b.seeds || a.units != b.units || a.quadrants != b.quadrants ||
            projected.resources.buffers != reference.resources.buffers ||
            projected.resources.missing != reference.resources.missing ||
            projected.resources.discarded != reference.resources.discarded) continue;
        const double gain = a.cash - b.cash;
        if (gain > best_gain) { best_gain = gain; best = trial; best_item = item; best_quantity = n; }
    }
    if (best_item >= 0) {
        memory.pending[best_item] = best_quantity;
        memory.delayed_units += best_quantity; ++memory.decisions;
        memory.predicted_gain += best_gain;
        memory.last_delay_turn = obs.turn; memory.last_target = obs.turn + 1;
        memory.last_item = best_item; memory.last_quantity = best_quantity;
    }
    return best;
}
}
