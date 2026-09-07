#include "agent.hpp"
#include <algorithm>
#include <cmath>

namespace compositions::atakan_portfolio {
namespace {
#include "data.inc"
double quote(int product,double inventory){
    const auto& p=kag::MARKET[product];const auto& a=kag::market_amps()[product];
    const double raw=inventory<p.I0?p.base+a.below*kag::shape(p.below_f,p.I0-inventory,p.T):p.base-a.above*kag::shape(p.above_f,inventory-p.I0,p.T);
    return std::max(1.,std::nearbyint(raw));
}
}
Estimate estimate(const kag::agent::AgentObservation& o,int branch){
    std::array<std::array<double,9>,30> rival{};
    for(const auto& row:o.opponent().tiles)for(const auto& tile:row)if(tile.has_animal){
        const auto& d=kag::ANIMALS[tile.what-kag::GOOSE];int pending=tile.pending_care_bonus;
        rival[o.day][d.product]+=tile.yield_units;
        for(int day=o.day+1;day<30;++day){
            const int age=day-tile.planted_day;
            if(age>=d.first_yield_day && (age-d.first_yield_day)%d.interval==0){rival[day][d.product]+=std::min(d.max_held,1+pending);pending=0;}
            ++pending;
        }
    }
    std::array<double,9> stock{},known{},expected{};
    for(int p=0;p<9;++p){
        stock[p]=o.market.inventory[p];known[p]=1;
        for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<p))known[p]+=6*kag::SHOP_MULT[o.shops[i]];
        for(int shop=0;shop<8;++shop)if(kag::SHOP_MASK[shop]&(1u<<p))expected[p]+=6.*kag::SHOP_MULT[shop]/8.;
    }
    Estimate result;double cash=o.self().money;result.min_cash=cash;
    for(int day=o.day;day<30;++day){
        double profit=-planned_fixed[branch][day];
        const double fraction=day==o.day?(24.-o.hour)/24.:1.;
        const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
        for(int p=0;p<9;++p){
            const double demand=fraction*(known[p]+.35*unseen*expected[p]);
            double sales=planned_sales[branch][day][p],buys=planned_buys[branch][day][p];
            const double internal=std::min(sales,buys);sales-=internal;buys-=internal;
            const double mid=stock[p]+.5*(sales+rival[day][p]-buys-demand);
            const double sell_price=quote(p,mid),buy_price=quote(p,mid-1);
            profit+=sales*sell_price-buys*buy_price;result.rival+=rival[day][p]*sell_price;
            stock[p]-=demand+buys;if(sell_price>1)stock[p]+=sales+rival[day][p];
        }
        result.own+=profit;cash+=profit;result.min_cash=std::min(result.min_cash,cash);
    }
    return result;
}
void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action){
    if(mode_<0 || mode_>6)std::abort();
    if(o.step==226){
        for(int branch=0;branch<3;++branch)estimates_[branch]=estimate(o,branch);
        if(mode_==3){
            int milk=0,wool=0;
            for(int i=0;i<o.n_shops;++i){const int shop=o.shops[i];milk+=(kag::SHOP_MASK[shop]&(1u<<kag::MILK))!=0;wool+=(shop==kag::SHOP_YARN_STORE);}
            branch_=wool?1:milk>=2?0:2;
        }else if(mode_==4)branch_=o.market.prices[kag::WOOL]>=195?1:o.market.prices[kag::MILK]>=180?0:2;
        else if(mode_>=5){
            branch_=0;double best=-1e100;
            for(int branch=0;branch<3;++branch){const auto& v=estimates_[branch];const double score=v.own-(mode_==6?v.rival:0);if(score>best){best=score;branch_=branch;}}
        }
    }
    action.clear();action.n_units=o.self().n_units;
    if(o.step<0 || o.step>=719){action.finalize();return;}
    int cursor=offsets[branch_][o.step];const int units=values[cursor++];action.n_orders=values[cursor++];
    for(int u=0;u<units;++u){action.units[u]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int u=units;u<action.n_units;++u)action.units[u]={};
    action.finalize();
}
}
