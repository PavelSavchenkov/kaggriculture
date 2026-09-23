#pragma once
#include "state.hpp"
#include <algorithm>

namespace kag::day_compiler {
struct StockSettlement {
    bool valid=false;
    double cash=0;
};
// Economic comparison only: use one actual solo market to restore another
// schedule's ending shed stock. Unbuyable deficits cannot be assigned a price.
inline StockSettlement settle_stock(const Sim& ending,const Farm& target,int seat) {
    const auto& own=ending.st.farms[seat];
    StockSettlement result{false,own.money};
    if(!std::equal(own.seeds,own.seeds+N_CROPS,target.seeds))return result;
    for(int p=GOOSE;p<N_ITEMS;++p)if(own.shed[p]!=target.shed[p])return result;
    for(int p=CARROT;p<=WOOL;++p)if(own.shed[p]<target.shed[p])return result;
    Action action; action.n_units=own.n_units;
    std::fill_n(action.units,action.n_units,UnitAction{});
    for(int p=0;p<N_PRODUCTS;++p)if(own.shed[p]>target.shed[p])
        action.orders[action.n_orders++]={M_SELL,uint8_t(p),own.shed[p]-target.shed[p]};
    for(int p:{WHEAT,FERTILIZER})if(own.shed[p]<target.shed[p])
        action.orders[action.n_orders++]={M_BUY_PRODUCT,uint8_t(p),target.shed[p]-own.shed[p]};
    action.finalize();
    if(!action.n_orders)return {true,own.money};
    auto sim=ending;
    const auto checked=sim.diagnose_solo_action(seat,action);
    if(checked.requested_order_units!=checked.successful_order_units)return result;
    if(seat)sim.step(Action{},action); else sim.step(action,Action{});
    const auto& final=sim.st.farms[seat];
    if(!std::equal(final.shed,final.shed+N_ITEMS,target.shed))return result;
    return {true,final.money};
}
}
