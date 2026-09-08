#pragma once
#include <array>
#include <algorithm>
#include "../../../../../../fast_game_engine/sim.hpp"

namespace catalog_early_melon_b98_m1_compositions {
inline constexpr int turns=719;
using ProductVector=std::array<int,kag::N_PRODUCTS>;

struct EconomicScenario {
    std::array<uint8_t,8> shops{};
    std::array<ProductVector,turns> rival_buys{},rival_sells{};
    double rival_fixed_cost=0;
};

struct FinancialPlan {
    // Arrivals are deposits before the market; use is earlier field supply
    // withdrawal. Purchases this turn supply next turn, never earlier actions.
    std::array<ProductVector,turns> arrivals{},use{};
    std::array<double,turns> fixed_cost{};
    std::array<int,turns> fixed_orders{};
    int reserve_days=1;
};

struct EconomicEstimate {
    double cash=3000, min_cash=3000, sales=0, purchases=0, fixed_cost=0;
    double rival_cash=3000,rival_sales=0,rival_purchases=0;
    int first_funding_step=-1, first_input_gap_step=-1, excess_order_slots=0;
    ProductVector bought{},sold{},discarded{},missing{},residue{},market{};
};

// An inexpensive conditional forecast against fixed rival flows. It preserves
// per-unit quotes and the $1 floor rule. Aggregate same-turn rival buys precede
// rival sales; original order-index interleaving is deliberately approximated.
// Negative cash is retained as an explicit funding witness, not hidden credit
// that makes a candidate feasible. Production is conditional on the plan.
inline EconomicEstimate economics(const FinancialPlan& plan,const EconomicScenario& scenario) {
    using namespace kag;
    if(plan.reserve_days<0 || plan.reserve_days>30) std::abort();
    for(auto shop:scenario.shops) if(shop>=N_SHOPS) std::abort();
    EconomicEstimate result;
    result.rival_cash-=scenario.rival_fixed_cost;
    ProductVector stock{},inventory{};
    inventory.fill(10000);
    int total=0;
    auto balance=[&](int step) {
        result.min_cash=std::min(result.min_cash,result.cash);
        if(result.cash<0 && result.first_funding_step<0) result.first_funding_step=step;
    };
    for(int step=0;step<turns;++step) {
        for(int item=0;item<N_PRODUCTS;++item) {
            const int requested=plan.use[step][item];
            const int used=std::min(stock[item],requested);
            stock[item]-=used;total-=used;
            if(used<requested) {
                result.missing[item]+=requested-used;
                if(result.first_input_gap_step<0) result.first_input_gap_step=step;
            }
        }
        for(int item=0;item<N_PRODUCTS;++item) {
            const int arrived=plan.arrivals[step][item];
            const int kept=std::min(arrived,std::max(0,100-total));
            stock[item]+=kept;total+=kept;
            result.discarded[item]+=arrived-kept;
        }
        ProductVector reserve{},buy{},sell{};
        const int end=std::min(turns,step+1+24*plan.reserve_days);
        for(int future=step+1;future<end;++future) {
            reserve[WHEAT]+=plan.use[future][WHEAT];
            reserve[FERTILIZER]+=plan.use[future][FERTILIZER];
        }
        for(int item=0;item<N_PRODUCTS;++item) {
            if(item==WHEAT || item==FERTILIZER) buy[item]=std::max(0,reserve[item]-stock[item]);
            sell[item]=std::max(0,stock[item]-reserve[item]);
        }
        int orders=plan.fixed_orders[step];
        for(int item=0;item<N_PRODUCTS;++item) orders+=(buy[item]>0)+(sell[item]>0);
        result.excess_order_slots+=std::max(0,orders-10);
        // First sell currently available surplus to finance planned purchases.
        // This is a candidate schedule choice, not a universal market rule.
        for(int item=0;item<N_PRODUCTS;++item) {
            const int rival_buy=scenario.rival_buys[step][item];
            for(int n=0;n<rival_buy;++n) {
                --inventory[item];
                const int price=market_price(item,inventory[item]);
                result.rival_cash-=price;result.rival_purchases+=price;
            }
            const int count=std::max(sell[item],scenario.rival_sells[step][item]);
            for(int n=0;n<count;++n) {
                const int price=market_price(item,inventory[item]);
                if(n<sell[item]) {
                    result.cash+=price;result.sales+=price;--stock[item];--total;++result.sold[item];
                    if(price>1)++inventory[item];
                }
                if(n<scenario.rival_sells[step][item]) {
                    result.rival_cash+=price;result.rival_sales+=price;
                    if(price>1)++inventory[item];
                }
            }
        }
        result.cash-=plan.fixed_cost[step];result.fixed_cost+=plan.fixed_cost[step];balance(step);
        for(int item : {int(WHEAT),int(FERTILIZER)}) {
            for(int n=0;n<buy[item] && total<100;++n) {
                const int price=market_price(item,inventory[item]-1);
                --inventory[item];++stock[item];++total;++result.bought[item];
                result.cash-=price;result.purchases+=price;balance(step);
            }
        }
        if(step%4==0) {
            const int shops=std::min(8,step/72);
            for(int s=0;s<shops;++s) {
                const int shop=scenario.shops[s];
                for(int item=0;item<N_PRODUCTS;++item)
                    if(SHOP_MASK[shop]&(1u<<item)) inventory[item]-=SHOP_MULT[shop];
            }
        }
        if(step%24==0) for(int item=0;item<FERTILIZER;++item) --inventory[item];
    }
    result.residue=stock;result.market=inventory;
    return result;
}
}
