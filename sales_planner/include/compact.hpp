#pragma once
#include "market.hpp"

namespace sales_planner {
// Remove only guaranteed no-ops, using an upper bound on available quantities.
// Uncertain affordability never makes a purchase a guaranteed no-op.
inline Orders compact_orders(const Account& account, const Orders& input, int mode) {
    using namespace kag;
    Orders output;
    auto upper = account.stock;
    for (int slot = 0; slot < input.count; ++slot) {
        const auto o = input.values[slot];
        bool valid = false;
        if (o.op == M_HIRE || o.op == M_BUY_LAND) valid = true;
        else if (o.n > 0) {
            if (o.op == M_SELL && is_product(o.item)) {
                valid = upper[o.item] > 0;
                upper[o.item] = std::max(0, upper[o.item] - o.n);
            } else if (o.op == M_BUY_PRODUCT && (o.item == WHEAT || o.item == FERTILIZER)) {
                valid = true; upper[o.item] = std::min(100, upper[o.item] + std::min(o.n, 100));
            } else if (o.op == M_BUY_ANIMAL && is_animal(o.item)) {
                valid = true; upper[o.item] = std::min(100, upper[o.item] + std::min(o.n, 100));
            } else if (o.op == M_BUY_SEED && is_crop(o.item)) valid = true;
        }
        if (valid) output.values[output.count++] = o;
    }
    if (mode >= 1)
        for (int slot = 0; slot < output.count; ++slot) {
            const auto o = output.values[slot];
            if (o.op != M_SELL) return input;
            if (mode == 2 && (o.item == WHEAT || o.item == FERTILIZER)) return input;
        }
    return output;
}
}
