#pragma once
#include "fast_game_engine/sim.hpp"
#include <array>
#include <algorithm>

namespace compositions {
struct AcceptedTrade {int op=0,item=0,n=0;};
struct MarketSlot {std::array<AcceptedTrade,2> trades{};std::array<double,2> fixed{};};
struct MarketStep {std::array<MarketSlot,10> slots{};std::array<int,2> order_count{};};
using MarketTape=std::array<MarketStep,719>;
struct MarketValue {
    std::array<double,2> cash{3000,3000},minimum{3000,3000};
    std::array<int,2> first_funding{-1,-1};
};

// Offline extraction retains exact order indices and jointly accepted amounts.
// Each prefix starts from the same full pre-turn state; night is suppressed only
// in diagnostic copies. Neither hidden state nor these copies enter an agent.
inline MarketStep accepted_market(const kag::Sim& before,const kag::Action (&actions)[2]) {
    MarketStep result;
    for(int p=0;p<2;++p)result.order_count[p]=actions[p].n_orders;
    const int count=std::max(actions[0].n_orders,actions[1].n_orders);
    if(count==0)return result;
    auto phase=[&](int n) {
        auto copy=before;copy.st.hour=0;
        kag::Action prefix[2]={actions[0],actions[1]};
        for(auto& a:prefix)a.n_orders=std::min(n,a.n_orders);
        copy.step(prefix[0],prefix[1]);return copy;
    };
    auto previous=phase(0);
    for(int i=0;i<count;++i) {
        auto current=phase(i+1);
        for(int p=0;p<2;++p)if(i<actions[p].n_orders) {
            const auto& order=actions[p].orders[i];
            const auto& old=previous.st.farms[p];const auto& now=current.st.farms[p];
            auto& slot=result.slots[i];
            if(order.op==kag::M_SELL) {
                const int n=now.sold_units[order.item]-old.sold_units[order.item];
                slot.trades[p]={order.op,order.item,n};
            } else if(order.op==kag::M_BUY_PRODUCT) {
                const int n=now.shed[order.item]-old.shed[order.item];
                slot.trades[p]={order.op,order.item,n};
            } else slot.fixed[p]=now.total_spend-old.total_spend;
            if(slot.trades[p].n<0 || slot.fixed[p]<0)std::abort();
        }
        previous=std::move(current);
    }
    return result;
}

// Conditional valuation of accepted quantities. Revalues both players at joint
// per-unit quotes, preserving order slots, town demand and the price-floor rule.
// Changed quantities can invalidate inventory, cash or the opponent's policy;
// this is a conditional estimate, not an executable financial certificate.
inline MarketValue value_market(const MarketTape& tape,const std::array<uint8_t,8>& shops) {
    using namespace kag;
    MarketValue result;std::array<int,N_PRODUCTS> inventory;inventory.fill(10000);
    for(int step=0;step<719;++step) {
        const auto& turn=tape[step];
        for(int i=0;i<std::max(turn.order_count[0],turn.order_count[1]);++i) {
            const auto& slot=turn.slots[i];
            for(int p=0;p<2;++p)result.cash[p]-=slot.fixed[p];
            for(int n=0;n<std::max(slot.trades[0].n,slot.trades[1].n);++n) {
                int quote[2]{};
                for(int p=0;p<2;++p) {
                    const auto& trade=slot.trades[p];if(n>=trade.n)continue;
                    if(trade.op!=M_SELL && trade.op!=M_BUY_PRODUCT)std::abort();
                    quote[p]=market_price(trade.item,inventory[trade.item]-(trade.op==M_BUY_PRODUCT));
                }
                for(int p=0;p<2;++p) {
                    const auto& trade=slot.trades[p];if(n>=trade.n)continue;
                    if(trade.op==M_BUY_PRODUCT) {result.cash[p]-=quote[p];--inventory[trade.item];}
                    else {result.cash[p]+=quote[p];if(quote[p]>1)++inventory[trade.item];}
                }
            }
            for(int p=0;p<2;++p) {
                result.minimum[p]=std::min(result.minimum[p],result.cash[p]);
                if(result.cash[p]<0 && result.first_funding[p]<0)result.first_funding[p]=step;
            }
        }
        if(step%4==0)for(int s=0;s<std::min(8,step/72);++s)
            for(int item=0;item<N_PRODUCTS;++item)if(SHOP_MASK[shops[s]]&(1u<<item))inventory[item]-=SHOP_MULT[shops[s]];
        if(step%24==0)for(int item=0;item<FERTILIZER;++item)--inventory[item];
    }
    return result;
}
}
