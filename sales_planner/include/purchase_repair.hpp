#pragma once
#include "baseline.hpp"

namespace sales_planner {
struct PurchaseRepairStats { int decisions=0, orders=0, units=0, rejected=0; };

// Current workers have already acted. Preserve all supplied market orders and
// append only inputs needed by the next turn's workers. The own-order projection
// assumes no rival trade; exact execution still decides funding and capacity.
inline Orders repair_purchases(const PlannerObservation& obs,std::span<const CalendarTurn> calendar,
                              const Orders& warm,const MarketRules& rules,PurchaseRepairStats& stats) {
    if(obs.turn+1>=int(calendar.size()))return warm;
    int end=warm.count;
    while(end && warm.values[end-1].op==kag::M_NONE)--end;
    if(end>=rules.max_orders)return warm;
    MarketState state;
    state.accounts[0]=obs.own;state.inventory=obs.inventory;state.turn=obs.turn;
    const auto required=needs(obs,calendar,1);
    // Only repair a deficit that remains even if every requested current buy
    // fills. This avoids buying duplicates when rival quotes improve a fill.
    auto upper_stock=obs.own.stock;
    std::array<int64_t,kag::N_CROPS> upper_seeds{};
    std::copy(obs.own.seeds.begin(),obs.own.seeds.end(),upper_seeds.begin());
    for(int k=0;k<warm.count;++k) {
        const auto o=warm.values[k];
        const int n=std::max(0,int(o.n));
        if(o.op==kag::M_BUY_SEED && o.item<kag::N_CROPS)upper_seeds[o.item]+=n;
        if((o.op==kag::M_BUY_PRODUCT || o.op==kag::M_BUY_ANIMAL) && o.item<kag::N_ITEMS)
            upper_stock[o.item]=std::min<int64_t>(rules.capacity,int64_t(upper_stock[o.item])+n);
        if(o.op==kag::M_SELL && o.item<kag::N_PRODUCTS)upper_stock[o.item]=std::max(0,upper_stock[o.item]-n);
    }
    Orders result=warm;result.count=end;
    int extra_units=0;
    auto add=[&](int op,int item,int n) {
        if(n<=0)return true;
        if(result.count>=rules.max_orders)return false;
        result.add(op,item,n);extra_units+=n;return true;
    };
    for(int item=0;item<kag::N_CROPS;++item)
        if(!add(kag::M_BUY_SEED,item,int(std::max<int64_t>(0,required.seeds[item]-upper_seeds[item])))){++stats.rejected;return warm;}
    for(int item:{int(kag::WHEAT),int(kag::FERTILIZER),int(kag::GOOSE),int(kag::COW),int(kag::SHEEP)})
        if(!add(kag::is_animal(item)?kag::M_BUY_ANIMAL:kag::M_BUY_PRODUCT,item,
                required.stock[item]-upper_stock[item])){++stats.rejected;return warm;}
    if(!extra_units)return warm;
    // Reject a partial repair that already fails in the own-order projection.
    state.accounts[0]=obs.own;state.accounts[1]=Account{};state.inventory=obs.inventory;
    trade(state,{result,Orders{}},rules);
    auto resources=obs.resources;
    apply(state.accounts[0],resources,calendar[obs.turn].after_market,rules.capacity);
    apply(state.accounts[0],resources,calendar[obs.turn+1].before_market,rules.capacity);
    for(int item=0;item<kag::N_ITEMS;++item)
        if(resources.missing[item]>obs.resources.missing[item]){++stats.rejected;return warm;}
    ++stats.decisions;stats.orders+=result.count-end;stats.units+=extra_units;
    return result;
}
}
