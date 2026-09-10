#pragma once
#include "market.hpp"

namespace sales_planner {
// Reorder existing sales only. Quantities, sale turns, and non-sale turns stay
// as supplied. The score is the observable rule seen in Otter's latest replays.
inline Orders sale_priority(const Account& account, const std::array<int,kag::N_PRODUCTS>& inventory,
                            const Orders& original) {
    for (int k=0;k<original.count;++k)
        if (original.values[k].op!=kag::M_SELL && original.values[k].op!=kag::M_NONE) return original;
    std::array<int,10> indices{}, scores{};
    auto remaining=account.stock;
    for (int k=0;k<original.count;++k) {
        indices[k]=k;
        const auto o=original.values[k];
        if (o.op==kag::M_SELL && o.item<kag::N_PRODUCTS) {
            const int quantity=std::min(std::max(0,int(o.n)),remaining[o.item]);
            scores[k]=quantity*kag::market_price(o.item,inventory[o.item]);
            remaining[o.item]-=quantity;
        }
    }
    std::stable_sort(indices.begin(),indices.begin()+original.count,[&](int a,int b){return scores[a]>scores[b];});
    Orders result=original;
    for (int k=0;k<original.count;++k) result.values[k]=original.values[indices[k]];
    return result;
}
}
