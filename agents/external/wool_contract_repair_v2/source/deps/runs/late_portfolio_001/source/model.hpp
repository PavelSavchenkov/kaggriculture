#pragma once
#include "../../../include/guarded_day.hpp"
#include "../../../include/animal_investment_value.hpp"

namespace catalog_wool_contract_repair_v2_compositions::late_portfolio {
struct Calendar {
    std::array<std::vector<GuardedDay>,2> days;
    std::array<FarmFlowPlan,2> flows;
    std::array<double,2> fixed_cost{};
};
inline void prepare(Calendar& calendar) {
    const auto& a=calendar.days[0][7];const auto& b=calendar.days[1][7];
    if(a.plan.day!=20 || a.tiles!=b.tiles || a.check!=b.check || a.shed!=b.shed || a.seeds!=b.seeds || a.quadrants!=b.quadrants)std::abort();
    for(int leaf=0;leaf<2;++leaf)for(const auto& day:calendar.days[leaf]) {
        int hired=0;
        for(int hour=0;hour<24;++hour)for(int slot=0;slot<day.plan.actions[hour].n_orders;++slot) {
            const auto& order=day.plan.actions[hour].orders[slot];
            const int item=order.item,n=order.n;
            if(order.op==kag::M_SELL)calendar.flows[leaf].sales[day.plan.day][item]+=n;
            else if(order.op==kag::M_BUY_PRODUCT)calendar.flows[leaf].buys[day.plan.day][item]+=n;
            else if(order.op==kag::M_BUY_SEED)calendar.fixed_cost[leaf]+=n*kag::CROPS[item].seed;
            else if(order.op==kag::M_BUY_ANIMAL)calendar.fixed_cost[leaf]+=n*kag::ANIMALS[item-kag::GOOSE].cost;
            else if(order.op==kag::M_HIRE)for(int i=0;i<n;++i)calendar.fixed_cost[leaf]+=kag::fib(hired++);
            else if(order.op==kag::M_BUY_LAND)std::abort(); // This slot has fixed owned land.
        }
    }
}
struct Forecast {
    ProductFlows demand{};
    bool berry=false;
};
inline uint64_t random(uint64_t& state) {
    state+=0x9e3779b97f4a7c15ULL;uint64_t x=state;
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;
    return x^(x>>31);
}
inline Forecast scenario(const kag::agent::AgentObservation& o,int sample) {
    Forecast result;std::array<int,8> shops{};
    for(int reveal=0;reveal<8;++reveal) {
        if(reveal<o.n_shops){shops[reveal]=o.shops[reveal];continue;}
        std::array<int,8> permutation{0,1,2,3,4,5,6,7};
        uint64_t state=uint64_t(sample/8+1)*0xd1b54a32d192ed03ULL ^ uint64_t(reveal+1)*0x94d049bb133111ebULL;
        for(int i=7;i>0;--i)std::swap(permutation[i],permutation[random(state)%(i+1)]);
        shops[reveal]=permutation[sample%8];
    }
    int berry=0;
    for(int reveal=0;reveal<6;++reveal)if(kag::SHOP_MASK[shops[reveal]]&(1u<<kag::STRAWBERRY))berry+=kag::SHOP_MULT[shops[reveal]];
    result.berry=berry>=4;
    for(int day=o.day;day<30;++day)for(int product=0;product<kag::N_PRODUCTS;++product) {
        double demand=product==kag::FERTILIZER?0:1;
        for(int reveal=0;reveal<std::min(8,day/3);++reveal)
            if(kag::SHOP_MASK[shops[reveal]]&(1u<<product))demand+=6*kag::SHOP_MULT[shops[reveal]];
        result.demand[day][product]=demand;
    }
    return result;
}
}
