#pragma once
#include "market.hpp"
#include "agents/common/api/observation.hpp"

namespace sales_planner {

// Products that cannot be purchased or consumed by farm work.
inline bool output_product(int item) { return item > kag::WHEAT && item < kag::FERTILIZER; }

inline Items requested_output_sales(const Account& after_work, const Orders& orders, int max_orders = 10) {
    auto available = after_work.stock;
    Items sold{};
    for (int slot = 0; slot < std::min(orders.count, max_orders); ++slot) {
        const auto o = orders.values[slot];
        if (o.op != kag::M_SELL || !output_product(o.item) || o.n <= 0) continue;
        const int n = std::min(available[o.item], o.n);
        available[o.item] -= n; sold[o.item] += n;
    }
    return sold;
}

// An upper bound on output collected during the last turn, from public tiles
// and worker positions. Ambiguous DIG/decay/harvest transitions count as a
// possible harvest. This intentionally overestimates rather than inventing
// access to private worker inventories or actions.
inline Items possible_harvest(const kag::agent::AgentObservation& before,
                              const kag::agent::AgentObservation& after) {
    using namespace kag;
    Items result{};
    const auto& old = before.opponent(); const auto& now = after.opponent();
    int workers[BOARD][BOARD]{};
    for (int u = 0; u < old.n_units; ++u) ++workers[old.pos_y[u]][old.pos_x[u]];
    const bool night = after.day != before.day;
    for (int y = 0; y < BOARD; ++y) for (int x = 0; x < BOARD; ++x) {
        const auto& a = old.tiles[y][x]; const auto& b = now.tiles[y][x];
        const int count = workers[y][x];
        if (!count || a.yield_units <= 0 || (a.kind != T_PLANT && !a.has_animal)) continue;
        const int item = a.has_animal ? ANIMALS[a.what - GOOSE].product : a.what;
        if (!output_product(item)) continue;
        const bool same = a.kind == b.kind && a.what == b.what && a.has_animal == b.has_animal && a.planted_day == b.planted_day;
        int quantity = a.yield_units;
        if (a.has_animal) {
            const auto& animal = ANIMALS[a.what - GOOSE];
            int night_yield = 0;
            const int since = after.day - a.planted_day - animal.first_yield_day;
            if (night && since >= 0 && since % animal.interval == 0)
                night_yield = std::min(animal.max_held, 1 + ((a.fed_today || count >= 2) ? a.pending_care_bonus : 0));
            if (same && b.yield_units > night_yield) continue;
        } else {
            const auto& crop = CROPS[item];
            const int age = before.day - a.planted_day;
            if (age < crop.first_yield_day) continue;
            if (!crop.ongoing) {
                if (same) continue;
                if (!a.watered_today && count >= 2 && age <= crop.max_yield_day)
                    quantity = std::min(crop.max_yield, quantity + 2);
            } else {
                int night_yield = 0;
                const int since = after.day - a.planted_day - crop.first_yield_day;
                if (night && since >= 0 && since % crop.interval == 0 && since / crop.interval < crop.max_yield)
                    night_yield = 2;
                if (same && b.yield_units > night_yield) continue;
            }
        }
        result[item] += quantity;
    }
    return result;
}

struct RivalStockHistory {
    Items upper{}, last_harvest_upper{}, last_sale_lower{};
    std::array<bool, kag::N_PRODUCTS> last_sale_exact{};
    int observed_turns = 0;

    // Requires history from the standard empty-inventory start. The caller
    // computes own successful output quantities from its post-work stock and
    // submitted orders. Sales cannot fail for cash and these products cannot
    // be bought. Only public observations and own information enter here.
    void observe(const kag::agent::AgentObservation& before,
                 const kag::agent::AgentObservation& after,
                 const Items& own_sold, const MarketRules& rules = {}) {
        if (before.step != observed_turns || after.step != before.step + 1) std::abort();
        last_harvest_upper = possible_harvest(before, after);
        for (int item = 1; item < kag::FERTILIZER; ++item) {
            int consumption = before.step % rules.town_interval == 0;
            if (before.step % rules.shop_interval == 0)
                for (int s = 0; s < before.n_shops; ++s)
                    if (kag::SHOP_MASK[before.shops[s]] & (1u << item)) consumption += kag::SHOP_MULT[before.shops[s]];
            const int after_trade = after.market.inventory[item] + consumption;
            const int residual = after_trade - before.market.inventory[item] - own_sold[item];
            last_sale_exact[item] = kag::market_price(item, after_trade) > 1;
            if (last_sale_exact[item] && residual < 0) std::abort();
            // At the price floor some sales do not enter market inventory.
            // Subtract only a lower bound; an empty market signal is not proof
            // that the opponent kept or sold all its goods.
            last_sale_lower[item] = std::max(0, residual);
            upper[item] = std::max(0, upper[item] + last_harvest_upper[item] - last_sale_lower[item]);
            if (after.day != before.day) upper[item] = std::min(upper[item], rules.capacity);
        }
        ++observed_turns;
    }
};
}
