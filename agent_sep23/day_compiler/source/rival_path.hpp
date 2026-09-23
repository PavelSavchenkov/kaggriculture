#pragma once
#include "forecast.hpp"
#include "funding.hpp"
#include "market.hpp"
#include <algorithm>

namespace kag::day_compiler {
// A causal projected rival path for complete-candidate checks. Arrays use
// absolute hours; forecasts consume the remaining horizon from observation.
struct RivalPath {
    FundingScenario flow;
    static bool needs_sale_funding(const Sim& sim,int seat,const Action& action,const Configuration& config) {
        const auto& own=sim.st.farms[seat];
        int hires=own.hires_today,quadrants=own.n_quadrants,inventory[N_PRODUCTS];
        std::copy_n(sim.st.market.inventory,N_PRODUCTS,inventory);
        double bill=0;bool sales=false;
        for(int k=0;k<action.n_orders;++k) {
            const auto order=action.orders[k];
            if(order.op==M_SELL)sales=true;
            else if(order.op==M_HIRE)bill+=config.hire_mult*double(fib(hires++));
            else if(order.op==M_BUY_SEED)bill+=order.n*CROPS[order.item].seed;
            else if(order.op==M_BUY_ANIMAL)bill+=order.n*ANIMALS[order.item-GOOSE].cost;
            else if(order.op==M_BUY_LAND&&quadrants<4)bill+=LAND_PRICES[quadrants++-1];
            else if(order.op==M_BUY_PRODUCT) {
                const auto purchase=transact(order.item,inventory[order.item],-order.n,0);
                bill-=purchase.own_cash;inventory[order.item]=purchase.inventory;
            }
        }
        return sales&&bill>own.money;
    }
    RivalPath(const Observation& current,const History& history,MarketMode mode,int hours,const Configuration& config,
              bool cover_visible,bool cover_wheat,bool cover_fertilizer=false) {
        const int remaining=hours-current.hour;
        for(int p=WHEAT;p<=FERTILIZER;++p) {
            if(p==FERTILIZER && !(cover_visible && cover_fertilizer && current.day!=29)) continue;
            if(p==WHEAT && !uses_wheat_dp(mode) && !(cover_visible && cover_wheat && current.day!=29)) continue;
            int predicted[24]{};
            if(uses_model(mode)) forecast_sales(current,history,p,remaining,predicted,config,uses_terminal_balance(mode));
            if(cover_visible && current.day!=29) cover_visible_supply(current,history,p,remaining,predicted,config);
            for(int h=current.hour;h<hours;++h) {
                flow.rival_sales[h][p]=predicted[h-current.hour];
                if(mode==MarketMode::SellerRecentFlow) for(const auto& sample:history.samples())
                    if(sample.step>=0 && sample.step<current.step && sample.step%24==h && sample.identifiable[p])
                        flow.rival_sales[h][p]=std::clamp(sample.lower[p],0,100);
            }
        }
    }
    Action prepare(Sim& sim,int seat,int hour,const Configuration& config,const Action* funding_orders=nullptr) const {
        auto& rival=sim.st.farms[seat^1]; std::fill_n(rival.shed,N_ITEMS,0); rival.shed_total=0;
        Action action; action.n_units=rival.n_units;
        int products[N_PRODUCTS]; for(int p=0;p<N_PRODUCTS;++p)products[p]=p;
        if(funding_orders && needs_sale_funding(sim,seat,*funding_orders,config)) {
            // Quantities do not determine order positions. For a current
            // funding check, expose the own earliest sales to the corresponding
            // rival sales instead of sheltering them behind unrelated products.
            auto first_sale=[&](int product) {
                for(int k=0;k<funding_orders->n_orders;++k)
                    if(funding_orders->orders[k].op==M_SELL && funding_orders->orders[k].item==product)return k;
                return 10;
            };
            std::sort(products,products+N_PRODUCTS,[&](int a,int b) {
                const int x=first_sale(a),y=first_sale(b); return x!=y?x<y:a<b;
            });
        }
        for(int p:products) {
            const int quantity=std::min(flow.rival_sales[hour][p],config.shed_capacity-rival.shed_total);
            if(!quantity) continue;
            rival.shed[p]=quantity; rival.shed_total+=quantity;
            action.orders[action.n_orders++]={M_SELL,uint8_t(p),quantity};
        }
        action.finalize(); return action;
    }
};
}
