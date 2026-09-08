#pragma once
// SPDX-License-Identifier: Apache-2.0
// C++ translation of TITAN scheduler.py MarketPath/optimize_lot, pinned release
// 7c50bbfb41027f31a2d4bc9470424e815f1fcef1. See LINEAGE.json for exact sources.
#include "fast_game_engine/sim.hpp"
#include <array>
#include <algorithm>
#include <limits>
#include <tuple>

namespace compositions::titan_lots {
struct Tranche {int step=0,quantity=0;auto operator<=>(const Tranche&)const=default;};
struct Plan {
    std::array<Tranche,9> sales{};int count=0;
    void add(int step,int quantity){if(count==9)std::abort();sales[count++]={step,quantity};}
    int at(int step)const{for(int i=0;i<count;++i)if(sales[i].step==step)return sales[i].quantity;return 0;}
    bool operator<(const Plan& p)const{return std::lexicographical_compare(sales.begin(),sales.begin()+count,p.sales.begin(),p.sales.begin()+p.count);}
    bool operator==(const Plan& p)const{return count==p.count && std::equal(sales.begin(),sales.begin()+count,p.sales.begin());}
};
struct Context {
    int item=0,quantity=0,inventory=10000,now=0,last=718,rival_quantity=0,minimum_now=0;
    std::array<int,4> dates{};int n_dates=0;
    std::array<int,8> shops{};int n_shops=0;
    int shop_interval=4,center_interval=24;
    Plan reference;
};
struct Score {int64_t relative=0,own=0,rival=0;int carry=0;};
struct Result {Plan plan;int64_t worst_gain=0,sum_gain=0;int plans=0;std::array<Score,5> scores{};int n_scenarios=0;};

class MarketPath {
    const Context& c_;
    std::array<int,1025> quotes_{};
    int quote(int inventory) {
        const int index=inventory-c_.inventory+512;
        if(index<0 || index>=int(quotes_.size()))std::abort();
        int& price=quotes_[index];if(!price)price=kag::market_price(c_.item,inventory);return price;
    }
    int absorption(int step)const {
        int n=0;
        if(step%c_.shop_interval==0)for(int k=0;k<c_.n_shops;++k)
            if(kag::SHOP_MASK[c_.shops[k]]&(1u<<c_.item))n+=kag::SHOP_MULT[c_.shops[k]];
        if(c_.item!=kag::FERTILIZER && step%c_.center_interval==0)++n;
        return n;
    }
    int64_t single(int& inventory,int quantity) {
        int64_t value=0;
        for(int n=0;n<quantity;++n){const int price=quote(inventory);value+=price;if(price>1)++inventory;}
        return value;
    }
public:
    explicit MarketPath(const Context& c):c_(c){}
    Score score(const Plan& plan,int scenario) {
        int inventory=c_.inventory,sold=0;
        Score result;
        const int end=c_.dates[c_.n_dates-1];
        for(int step=c_.now;step<=end;++step) {
            const int own=std::min(c_.quantity-sold,std::max(0,plan.at(step)));
            const int rival_step=scenario==3?c_.now+1:scenario==4?end-1:c_.now;
            const int rival=scenario && step==rival_step?c_.rival_quantity:0;
            if(scenario==2) {
                result.own+=single(inventory,own);result.rival+=single(inventory,rival);
            }else for(int k=0;k<std::max(own,rival);++k) {
                const int price=quote(inventory);const int a=k<own,b=k<rival;
                result.own+=price*a;result.rival+=price*b;if(price>1)inventory+=a+b;
            }
            sold+=own;inventory-=absorption(step);
        }
        result.carry=c_.quantity-sold;
        const int64_t carry=end==c_.last?0:single(inventory,result.carry);
        result.relative=result.own+carry-result.rival;
        return result;
    }
};

// Capacity and funding are caller constraints. Every candidate passes capacity;
// minimum_now encodes required immediate receipts as a quantity bound. Runtime
// integration must provide a real receipt/storage contract, not assume one.
template<class Capacity> Result optimize(const Context& c,Capacity capacity) {
    if(c.item<0 || c.item>=kag::N_PRODUCTS || c.quantity<0 || c.quantity>100 || c.rival_quantity<0 || c.rival_quantity>100 ||
       c.n_dates<1 || c.n_dates>4 || c.dates[0]!=c.now || c.dates[c.n_dates-1]-c.now>8 ||
       c.n_shops<0 || c.n_shops>8 || c.minimum_now<0 || c.minimum_now>c.quantity)std::abort();
    for(int i=1;i<c.n_dates;++i)if(c.dates[i]<=c.dates[i-1])std::abort();
    Result result;result.plan=c.reference;
    const int end=c.dates[c.n_dates-1];
    result.n_scenarios=3+(end>c.now)+(end>c.now+2);
    MarketPath path(c);std::array<Score,5> baseline;
    for(int s=0;s<result.n_scenarios;++s)baseline[s]=result.scores[s]=path.score(c.reference,s);
    const bool reference_feasible=capacity(c.reference) && c.reference.at(c.now)>=c.minimum_now;
    std::tuple<int64_t,int64_t,int> best=reference_feasible?std::tuple<int64_t,int64_t,int>{0,0,0}:
        std::tuple<int64_t,int64_t,int>{std::numeric_limits<int64_t>::min(),std::numeric_limits<int64_t>::min(),0};
    std::array<Plan,810> plans;int count=0;
    auto add=[&](const Plan& p){if(count==int(plans.size()))std::abort();plans[count++]=p;};
    add(c.reference);
    for(int first=c.minimum_now;first<=c.quantity;++first) {
        const int remaining=c.quantity-first;Plan p;p.add(c.now,first);add(p);
        for(int j=1;j<c.n_dates;++j){Plan q=p;q.add(c.dates[j],remaining);add(q);}
        if(c.n_dates>=3)for(int share=1;share<=3;++share) {
            Plan q=p;const int a=remaining*share/4;q.add(c.dates[1],a);q.add(end,remaining-a);add(q);
        }
    }
    std::sort(plans.begin(),plans.begin()+count);
    count=std::unique(plans.begin(),plans.begin()+count)-plans.begin();result.plans=count;
    bool feasible=reference_feasible;
    for(int i=0;i<count;++i) {
        const auto& plan=plans[i];int total=0;for(int j=0;j<plan.count;++j)total+=plan.sales[j].quantity;
        if(total>c.quantity || plan.at(c.now)<c.minimum_now || !capacity(plan))continue;
        std::array<Score,5> scores;
        int64_t worst=std::numeric_limits<int64_t>::max(),sum=0;
        for(int s=0;s<result.n_scenarios;++s) {
            scores[s]=path.score(plan,s);const auto gain=scores[s].relative-baseline[s].relative;
            worst=std::min(worst,gain);sum+=gain;
        }
        const auto key=std::tuple{worst,sum,plan.at(c.now)};
        if((worst>0 || !reference_feasible) && key>best){best=key;result.plan=plan;result.scores=scores;feasible=true;}
    }
    if(feasible){result.worst_gain=std::get<0>(best);result.sum_gain=std::get<1>(best);}
    return result;
}
}
