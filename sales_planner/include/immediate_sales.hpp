#pragma once
#include "market.hpp"

namespace sales_planner {
// Latest top-player discovery suggests selling finished goods as soon as they
// reach the shed. Wheat and fertilizer remain under the plan's input policy.
// This primitive preserves inherited order positions and purchase quantities.
inline Orders immediate_sales(const Account& account, const std::array<int,kag::N_PRODUCTS>& inventory,
                              const Orders& original, const MarketRules& rules, bool sales_only) {
    if (sales_only)
        for (int k=0;k<original.count;++k)
            if (original.values[k].op != kag::M_SELL && original.values[k].op != kag::M_NONE) return original;
    Orders result=original;
    std::array<bool,kag::N_PRODUCTS> included{};
    for (int k=0;k<result.count;++k) {
        auto& o=result.values[k];
        if (o.op!=kag::M_SELL || o.item==kag::WHEAT || o.item>=kag::FERTILIZER) continue;
        o.n=included[o.item]?0:account.stock[o.item];
        included[o.item]=true;
    }
    std::array<int,7> products{1,2,3,4,5,6,7};
    std::stable_sort(products.begin(),products.end(),[&](int a,int b){
        return kag::market_price(a,inventory[a])>kag::market_price(b,inventory[b]);
    });
    for (int item:products)
        if (!included[item] && account.stock[item]>0 && result.count<rules.max_orders)
            result.add(kag::M_SELL,item,account.stock[item]);
    return result;
}
}
