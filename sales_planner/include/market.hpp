#pragma once
#include "fast_game_engine/sim.hpp"
#include <array>
#include <span>

namespace sales_planner {
inline constexpr int terminal_turn = 719;
using Items = std::array<int, kag::N_ITEMS>;

struct Account {
    double cash = 3000;
    Items stock{};
    std::array<int, kag::N_CROPS> seeds{};
    int total = 0, units = 1, hires = 0, quadrants = 1;
};

struct MarketState {
    std::array<Account, 2> accounts{};
    std::array<int, kag::N_PRODUCTS> inventory{};
    std::array<uint8_t, kag::N_SHOPS> shops{};
    int n_shops = 0, turn = 0;
    MarketState() { inventory.fill(10000); }
};

struct MarketRules {
    int capacity = 100, max_orders = 10, hire_mult = 1;
    int turns_per_day = 24, shop_interval = 4, town_interval = 24;
};

struct Orders {
    std::array<kag::Order, 16> values{};
    int count = 0;
    void add(uint8_t op, uint8_t item = 0, int n = 1) {
        if (count == int(values.size())) std::abort();
        values[count++] = {op, item, n};
    }
};

struct TradeResult {
    std::array<std::array<int, 16>, 2> accepted{};
    std::array<double, 2> receipts{}, spending{};
};

// Exact order-slot and per-unit interleaving. This layer has no future scenario.
inline TradeResult trade(MarketState& s, const std::array<Orders, 2>& orders,
                         const MarketRules& rules = {}) {
    using namespace kag;
    TradeResult result;
    const int slots = std::min(rules.max_orders, std::max(orders[0].count, orders[1].count));
    for (int slot = 0; slot < slots; ++slot) {
        Order active[2]{};
        for (int p = 0; p < 2; ++p) {
            if (slot >= orders[p].count) continue;
            active[p] = orders[p].values[slot];
            auto& a = s.accounts[p];
            const int op = active[p].op;
            if (op == M_HIRE || op == M_BUY_LAND) {
                const bool legal = op == M_HIRE ? a.units < MAX_UNITS : a.quadrants < 4;
                const int price = op == M_HIRE ? rules.hire_mult * fib(a.hires) :
                    (legal ? LAND_PRICES[a.quadrants - 1] : 0);
                if (legal && a.cash >= price) {
                    a.cash -= price;
                    result.spending[p] += price;
                    result.accepted[p][slot] = 1;
                    if (op == M_HIRE) { ++a.units; ++a.hires; } else ++a.quadrants;
                }
                active[p].n = 0;
            }
        }
        for (;;) {
            int quote[2]{}, valid[2]{};
            for (int p = 0; p < 2; ++p) {
                const auto o = active[p];
                if (o.n <= 0) continue;
                if (o.op == M_SELL && is_product(o.item))
                    quote[p] = market_price(o.item, s.inventory[o.item]);
                else if (o.op == M_BUY_PRODUCT && (o.item == WHEAT || o.item == FERTILIZER))
                    quote[p] = market_price(o.item, s.inventory[o.item] - 1);
                else if (o.op == M_BUY_SEED && is_crop(o.item)) quote[p] = CROPS[o.item].seed;
                else if (o.op == M_BUY_ANIMAL && is_animal(o.item)) quote[p] = ANIMALS[o.item - GOOSE].cost;
                else { active[p].n = 0; continue; }
                valid[p] = 1;
            }
            if (!valid[0] && !valid[1]) break;
            for (int p = 0; p < 2; ++p) {
                if (!valid[p]) continue;
                auto& a = s.accounts[p];
                auto& o = active[p];
                const int item = o.item, price = quote[p];
                if (o.op == M_SELL) {
                    if (a.stock[item] == 0) { o.n = 0; continue; }
                    --a.stock[item]; --a.total;
                    a.cash += price; result.receipts[p] += price;
                    if (price > 1) ++s.inventory[item];
                } else {
                    if (a.cash < price || (o.op != M_BUY_SEED && a.total >= rules.capacity)) {
                        o.n = 0; continue;
                    }
                    a.cash -= price; result.spending[p] += price;
                    if (o.op == M_BUY_SEED) ++a.seeds[item];
                    else { ++a.stock[item]; ++a.total; }
                    if (o.op == M_BUY_PRODUCT) --s.inventory[item];
                }
                ++result.accepted[p][slot]; --o.n;
            }
        }
    }
    return result;
}

// Called after market work, before day-end resource events. New shops are
// revealed by the caller at the start of their unlock day, never in advance.
inline void consume(MarketState& s, const MarketRules& rules = {}) {
    if (s.turn % rules.shop_interval == 0)
        for (int i = 0; i < s.n_shops; ++i)
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                if (kag::SHOP_MASK[s.shops[i]] & (1u << item))
                    s.inventory[item] -= kag::SHOP_MULT[s.shops[i]];
    if (s.turn % rules.town_interval == 0)
        for (int item = 0; item < kag::FERTILIZER; ++item) --s.inventory[item];
}

inline void advance(MarketState& s, const MarketRules& rules = {}) {
    ++s.turn;
    if (s.turn % rules.turns_per_day == 0)
        for (auto& a : s.accounts) { a.units = 1; a.hires = 0; }
}

inline MarketState financial_state(const kag::State& state) {
    MarketState s;
    s.turn = state.step; s.n_shops = state.n_shops;
    std::copy_n(state.shops, s.n_shops, s.shops.begin());
    std::copy_n(state.market.inventory, kag::N_PRODUCTS, s.inventory.begin());
    for (int p = 0; p < 2; ++p) {
        const auto& f = state.farms[p]; auto& a = s.accounts[p];
        a.cash = f.money; a.total = f.shed_total; a.units = f.n_units;
        a.hires = f.hires_today; a.quadrants = f.n_quadrants;
        std::copy_n(f.shed, kag::N_ITEMS, a.stock.begin());
        std::copy_n(f.seeds, kag::N_CROPS, a.seeds.begin());
    }
    return s;
}
}
