#include "agent.hpp"
#include <algorithm>
#include <cmath>

namespace compositions::atakan_integrated {
namespace {
#include "data.inc"
using Products=std::array<double,9>;
using Days=std::array<Products,30>;
uint64_t random(uint64_t&state){state+=0x9e3779b97f4a7c15ULL;uint64_t x=state;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
double quote(int product,double inventory){const auto&p=kag::MARKET[product];const auto&a=kag::market_amps()[product];
    const double raw=inventory<p.I0?p.base+a.below*kag::shape(p.below_f,p.I0-inventory,p.T):p.base-a.above*kag::shape(p.above_f,inventory-p.I0,p.T);return std::max(1.,std::nearbyint(raw));}
Days rival_flow(const kag::agent::AgentObservation&o){Days rival{};
    for(const auto&row:o.opponent().tiles)for(const auto&tile:row)if(tile.has_animal){const auto&d=kag::ANIMALS[tile.what-kag::GOOSE];int pending=tile.pending_care_bonus;rival[o.day][d.product]+=tile.yield_units;
        for(int day=o.day+1;day<30;++day){const int age=day-tile.planted_day;if(age>=d.first_yield_day&&(age-d.first_yield_day)%d.interval==0){rival[day][d.product]+=std::min(d.max_held,1+pending);pending=0;}++pending;}}
    return rival;
}
Days mean_demand(const kag::agent::AgentObservation&o,double discount){Days result{};Products known{},expected{};
    for(int p=0;p<9;++p){known[p]=1;for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<p))known[p]+=6*kag::SHOP_MULT[o.shops[i]];
        for(int shop=0;shop<8;++shop)if(kag::SHOP_MASK[shop]&(1u<<p))expected[p]+=6.*kag::SHOP_MULT[shop]/8.;}
    for(int day=o.day;day<30;++day){const double fraction=day==o.day?(24.-o.hour)/24.:1.;const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
        for(int p=0;p<9;++p)result[day][p]=fraction*(known[p]+discount*unseen*expected[p]);}return result;
}
Days sample_demand(const kag::agent::AgentObservation&o,int sample){std::array<int,8>shops{};
    for(int reveal=0;reveal<8;++reveal){if(reveal<o.n_shops){shops[reveal]=o.shops[reveal];continue;}
        std::array<int,8>permutation{0,1,2,3,4,5,6,7};uint64_t state=uint64_t(sample/8+1)*0xd1b54a32d192ed03ULL^uint64_t(reveal+1)*0x94d049bb133111ebULL;
        for(int i=7;i>0;--i)std::swap(permutation[i],permutation[random(state)%(i+1)]);shops[reveal]=permutation[sample%8];}
    Days result{};
    for(int day=o.day;day<30;++day){const double fraction=day==o.day?(24.-o.hour)/24.:1.;const int reveals=std::max(o.n_shops,std::min(8,day/3));
        for(int p=0;p<9;++p){double demand=1;for(int reveal=0;reveal<reveals;++reveal)if(kag::SHOP_MASK[shops[reveal]]&(1u<<p))demand+=6*kag::SHOP_MULT[shops[reveal]];result[day][p]=fraction*demand;}}
    return result;
}
Estimate value(const kag::agent::AgentObservation&o,int branch,const Days&rival,const Days&demand){Products stock{};for(int p=0;p<9;++p)stock[p]=o.market.inventory[p];
    Estimate result;double cash=o.self().money;result.min_cash=cash;
    for(int day=o.day;day<30;++day){double profit=-planned_fixed[branch][day];
        for(int p=0;p<9;++p){double sales=planned_sales[branch][day][p],buys=planned_buys[branch][day][p];const double internal=std::min(sales,buys);sales-=internal;buys-=internal;
            const double mid=stock[p]+.5*(sales+rival[day][p]-buys-demand[day][p]),sell_price=quote(p,mid),buy_price=quote(p,mid-1);
            profit+=sales*sell_price-buys*buy_price;result.rival+=rival[day][p]*sell_price;stock[p]-=demand[day][p]+buys;if(sell_price>1)stock[p]+=sales+rival[day][p];
        }result.own+=profit;cash+=profit;result.min_cash=std::min(result.min_cash,cash);
    }return result;
}
}
std::array<Estimate,3>estimate_all(const kag::agent::AgentObservation&o,int count){
    if(count!=0&&count!=1&&count!=8&&count!=32&&count!=64)std::abort();const Days rival=rival_flow(o);std::array<Estimate,3>result{};
    const int n=std::max(1,count);
    for(int sample=0;sample<n;++sample){const Days demand=count<2?mean_demand(o,count?1.:.35):sample_demand(o,sample);
        for(int branch=0;branch<3;++branch){const auto v=value(o,branch,rival,demand);result[branch].own+=v.own;result[branch].rival+=v.rival;result[branch].min_cash+=v.min_cash;}}
    for(auto&v:result){v.own/=n;v.rival/=n;v.min_cash/=n;}return result;
}
void Agent::act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&,kag::Action&action){
    if(o.step==226){estimates_=estimate_all(o,count_);branch_=0;double best=-1e100;for(int branch=0;branch<3;++branch){const auto&v=estimates_[branch];const double score=v.own-(margin_?v.rival:0);if(score>best){best=score;branch_=branch;}}}
    action.clear();action.n_units=o.self().n_units;if(o.step<0||o.step>=719){action.finalize();return;}
    int cursor=offsets[branch_][o.step];const int units=values[cursor++];action.n_orders=values[cursor++];
    for(int u=0;u<units;++u){action.units[u]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int u=units;u<action.n_units;++u)action.units[u]={};action.finalize();
}
}
