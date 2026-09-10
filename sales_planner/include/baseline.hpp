#pragma once
#include "calendar.hpp"

namespace sales_planner {
struct PlannerObservation {
    int turn = 0;
    Account own;
    Resources resources;
    std::array<int, kag::N_PRODUCTS> inventory{};
    std::array<uint8_t, 8> shops{};
    int n_shops = 0;
    double rival_cash = 0;
};
struct Needs {
    Items stock{};
    std::array<int, kag::N_CROPS> seeds{};
};

// Minimum current stock needed if there are no purchases before the chosen
// deadline. Production and input use are conditional on the supplied calendar.
// Infinite capacity here estimates reservations; real capacity is checked in
// the exact rollout and can make a heuristic candidate invalid.
inline Needs needs(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                   int horizon, bool include_current_before = false) {
    Needs result;
    Account a; a.cash = 0;
    Resources r = obs.resources;
    auto events = [&](std::span<const ResourceEvent> flows) {
        for (const auto e : flows) {
            if (e.flow == Flow::withdraw) {
                const int shortfall = std::max(0, e.quantity - a.stock[e.item]);
                result.stock[e.item] += shortfall; a.stock[e.item] += shortfall; a.total += shortfall;
            } else if (e.flow == Flow::use_seed) {
                const int shortfall = std::max(0, e.quantity - a.seeds[e.item]);
                result.seeds[e.item] += shortfall; a.seeds[e.item] += shortfall;
            }
            apply(a, r, std::span<const ResourceEvent>(&e, 1), 1000000);
        }
    };
    const int end = std::min(int(calendar.size()) - 1, obs.turn + horizon);
    for (int t = obs.turn; t <= end; ++t) {
        if (t > obs.turn || include_current_before) events(calendar[t].before_market);
        if (t < end) events(calendar[t].after_market);
    }
    return result;
}

struct BaselineOptions {
    int reserve_horizon = 24, buy_horizon = 8, seed_horizon = 24;
    bool preserve_purchases = false;
    bool bridge_cash = false;
    bool retain_unbuyable = false;
    bool urgent_first = false;
};

inline Orders baseline_orders(const PlannerObservation& obs,
                              std::span<const CalendarTurn> calendar,
                              const MarketRules& rules, const BaselineOptions& options,
                              const Orders* warm_orders = nullptr) {
    using namespace kag;
    Orders result = calendar[obs.turn].commitments;
    auto reserve = needs(obs, calendar, options.reserve_horizon);
    auto buying = needs(obs, calendar, options.buy_horizon);
    auto seeds = needs(obs, calendar, options.seed_horizon);
    if (options.retain_unbuyable) {
        // A small conservative reserve for supplied future transfers of items
        // that cannot be repurchased. Subsequent arrivals may make this loose.
        Items future_withdrawals{};
        for (int t = obs.turn + 1; t < int(calendar.size()); ++t)
            for (const auto e : calendar[t].before_market)
                if (e.flow == Flow::withdraw) future_withdrawals[e.item] += e.quantity;
        for (int item = 0; item < N_PRODUCTS; ++item)
            if (item != WHEAT && item != FERTILIZER)
                reserve.stock[item] = std::max(reserve.stock[item], future_withdrawals[item]);
    }
    if (options.bridge_cash) {
        double costs = 0;
        int hires = obs.own.hires, quadrants = obs.own.quadrants;
        for (int t = obs.turn; t < std::min(int(calendar.size()), obs.turn + 24); ++t) {
            if (t != obs.turn && t % rules.turns_per_day == 0) hires = 0;
            for (int slot = 0; slot < calendar[t].commitments.count; ++slot) {
                const auto o = calendar[t].commitments.values[slot];
                if (o.op == M_HIRE) costs += rules.hire_mult * fib(hires++);
                if (o.op == M_BUY_LAND && quadrants < 4) costs += LAND_PRICES[quadrants++ - 1];
            }
        }
        const auto next = needs(obs, calendar, 2);
        for (int i = 0; i < N_CROPS; ++i) costs += CROPS[i].seed * std::max(0, next.seeds[i] - obs.own.seeds[i]);
        for (int item : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)}) {
            const int price = is_animal(item) ? ANIMALS[item - GOOSE].cost : market_price(item, obs.inventory[item] - 1);
            costs += price * std::max(0, next.stock[item] - obs.own.stock[item]);
        }
        if (obs.own.cash < costs) {
            reserve = needs(obs, calendar, 1);
            buying = seeds = next;
        }
    }
    if (options.preserve_purchases) {
        if (!warm_orders) std::abort();
        for (int slot = 0; slot < warm_orders->count; ++slot) {
            const auto o = warm_orders->values[slot];
            if (o.op == M_BUY_SEED || o.op == M_BUY_PRODUCT || o.op == M_BUY_ANIMAL)
                result.values[slot] = o;
        }
        result.count = std::max(result.count, warm_orders->count);
    }
    auto insert = [&](uint8_t op, int item, int n) {
        if (n <= 0) return false;
        for (int slot = 0; slot < rules.max_orders; ++slot)
            if (slot >= result.count || result.values[slot].op == M_NONE) {
                result.values[slot] = {op, uint8_t(item), n};
                result.count = std::max(result.count, slot + 1);
                return true;
            }
        return false;
    };
    std::array<int, N_PRODUCTS> products;
    for (int item = 0; item < N_PRODUCTS; ++item) products[item] = item;
    std::stable_sort(products.begin(), products.end(), [&](int a, int b) {
        return market_price(a, obs.inventory[a]) > market_price(b, obs.inventory[b]);
    });
    Items early_sold{}, early_bought{};
    std::array<int, N_CROPS> early_seeds{};
    if (options.urgent_first && !options.preserve_purchases) {
        const auto urgent = needs(obs, calendar, 1);
        double expense = 0;
        int hires = obs.own.hires, quadrants = obs.own.quadrants;
        for (int slot = 0; slot < result.count; ++slot) {
            const auto o = result.values[slot];
            if (o.op == M_HIRE) expense += rules.hire_mult * fib(hires++);
            if (o.op == M_BUY_LAND && quadrants < 4) expense += LAND_PRICES[quadrants++ - 1];
        }
        for (int i = 0; i < N_CROPS; ++i) expense += CROPS[i].seed * std::max(0, urgent.seeds[i] - obs.own.seeds[i]);
        for (int item : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)}) {
            const int price = is_animal(item) ? ANIMALS[item - GOOSE].cost : market_price(item, obs.inventory[item] - 1);
            expense += price * std::max(0, urgent.stock[item] - obs.own.stock[item]);
        }
        // Raise required cash before placing urgent purchases. Exact replay is
        // still the feasibility judge when a rival moves simultaneous quotes.
        double cash = obs.own.cash;
        for (int item : products) {
            if (cash >= expense) break;
            const int n = std::max(0, obs.own.stock[item] - reserve.stock[item]);
            if (insert(M_SELL, item, n)) {
                early_sold[item] = n;
                for (int k = 0; k < n; ++k) cash += market_price(item, obs.inventory[item] + k);
            }
        }
        for (int item = 0; item < N_CROPS; ++item) {
            const int n = std::max(0, urgent.seeds[item] - obs.own.seeds[item]);
            if (insert(M_BUY_SEED, item, n)) early_seeds[item] = n;
        }
        for (int item : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)}) {
            const int n = std::max(0, urgent.stock[item] - obs.own.stock[item]);
            if (insert(is_animal(item) ? M_BUY_ANIMAL : M_BUY_PRODUCT, item, n)) early_bought[item] = n;
        }
    }
    for (int item : products) insert(M_SELL, item, obs.own.stock[item] - reserve.stock[item] - early_sold[item]);
    if (!options.preserve_purchases) {
        // Buy earliest-deadline obligations first; a short horizon is a cold
        // reference, not a feasibility proof. Exact rollout reports shortfalls.
        const auto urgent = needs(obs, calendar, 1);
        struct Buy { uint8_t op, item; int n, urgent; };
        std::array<Buy, N_CROPS + N_ANIMALS + 2> candidates;
        int count = 0;
        for (int item = 0; item < N_CROPS; ++item)
            candidates[count++] = {M_BUY_SEED, uint8_t(item), std::max(0, seeds.seeds[item] - obs.own.seeds[item] - early_seeds[item]),
                                  std::max(0, urgent.seeds[item] - obs.own.seeds[item])};
        for (int item : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)})
            candidates[count++] = {uint8_t(is_animal(item) ? M_BUY_ANIMAL : M_BUY_PRODUCT), uint8_t(item),
                                  std::max(0, buying.stock[item] - obs.own.stock[item] - early_bought[item]),
                                  std::max(0, urgent.stock[item] - obs.own.stock[item])};
        std::stable_sort(candidates.begin(), candidates.begin() + count, [](auto a, auto b) {
            return a.urgent > b.urgent;
        });
        for (int i = 0; i < count; ++i) insert(candidates[i].op, candidates[i].item, candidates[i].n);
    }
    while (result.count && result.values[result.count - 1].op == M_NONE) --result.count;
    return result;
}
}
