#pragma once
#include "baseline.hpp"
#include "compact.hpp"

namespace sales_planner {
struct StorageStats { int decisions = 0, sold = 0, predicted_saved = 0, evaluations = 0; };

// The observation is after the current worker actions. For sale-only orders,
// accepted quantities and the following deposits do not depend on rival quotes.
// Mode 0 keeps every post-deposit item; other modes keep calendar input reserves.
inline Orders storage_orders(const PlannerObservation& obs, std::span<const ResourceEvent> after_market,
                             const Orders& warm, const MarketRules& rules, StorageStats& stats,
                             std::span<const CalendarTurn> calendar = {}, int horizon = 0) {
    const auto original = compact_orders(obs.own, warm, 1);
    if (after_market.empty() || original.count >= rules.max_orders) return original;
    for (int k = 0; k < original.count; ++k)
        if (original.values[k].op != kag::M_SELL) return original;
    auto market = MarketState{};
    market.turn = obs.turn; market.accounts[0] = obs.own; market.inventory = obs.inventory;
    market.shops = obs.shops; market.n_shops = obs.n_shops;
    trade(market, {original, Orders{}}, rules);
    const auto post_market = market.accounts[0];
    auto account = post_market; auto resources = obs.resources;
    apply(account, resources, after_market, rules.capacity);
    int lost = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        lost += resources.discarded[item] - obs.resources.discarded[item];
    if (!lost) return original;
    Items reserve = account.stock;
    if (horizon) {
        auto next = obs; ++next.turn; next.own = account; next.resources = resources;
        reserve = needs(next, calendar, horizon, true).stock;
        // Deficits already present in the reference cannot be repaired here.
        for (int item = 0; item < kag::N_ITEMS; ++item)
            reserve[item] = std::min(reserve[item], account.stock[item]);
    }
    Orders best = original;
    double best_value = 0;
    int best_sold = 0, best_saved = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        double receipts = 0;
        for (int q = 1; q <= std::min(lost, post_market.stock[item]); ++q) {
            receipts += kag::market_price(item, market.inventory[item] + q - 1);
            ++stats.evaluations;
            auto a = post_market; auto r = obs.resources;
            a.stock[item] -= q; a.total -= q;
            apply(a, r, after_market, rules.capacity);
            bool keeps_inputs = true;
            int saved = 0;
            double value = receipts;
            for (int i = 0; i < kag::N_ITEMS; ++i) {
                if (a.stock[i] < reserve[i]) keeps_inputs = false;
                saved += resources.discarded[i] - r.discarded[i];
                if (i < kag::N_PRODUCTS)
                    value += (a.stock[i] - account.stock[i]) * kag::market_price(i, market.inventory[i]);
            }
            if (!keeps_inputs || saved <= 0 || value <= best_value) continue;
            best = original; best.add(kag::M_SELL, item, q);
            best_value = value; best_sold = q; best_saved = saved;
        }
    }
    if (best_sold) { ++stats.decisions; stats.sold += best_sold; stats.predicted_saved += best_saved; }
    return best;
}

inline Orders make_room(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                        const Orders& warm, const MarketRules& rules, StorageStats& stats, int horizon = 0) {
    return storage_orders(obs, calendar[obs.turn].after_market, warm, rules, stats, calendar, horizon);
}
}
