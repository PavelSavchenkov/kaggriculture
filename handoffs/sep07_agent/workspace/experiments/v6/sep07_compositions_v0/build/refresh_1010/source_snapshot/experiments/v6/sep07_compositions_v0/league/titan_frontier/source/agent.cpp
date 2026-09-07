// Apache-2.0. Modified: typed C++ port of Kaito v43 and Igor/LARK helpers.
// Exact external source and module lineage are recorded in ../IMPORT.json.
#include "agent.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace compositions::titan_frontier {
namespace {
using namespace kag;
using Observation=agent::AgentObservation;
using Configuration=agent::AgentConfig;
#include "routes.inc"

void route_action(int route,int step,int units,Action& action) {
    const int* data=route_values+route_offsets[route][std::clamp(step,0,718)];
    const int recorded=*data++,orders=*data++;
    action.clear();action.n_units=units;std::fill_n(action.units,units,UnitAction{});
    for(int u=0;u<recorded;++u,data+=3)if(u<units)action.units[u]={uint8_t(data[0]),uint8_t(data[1]),data[2]};
    for(int m=0;m<orders;++m,data+=3)action.orders[action.n_orders++]={uint8_t(data[0]),uint8_t(data[1]),data[2]};
}

std::array<int,15> counts(const agent::PublicFarm& farm) {
    std::array<int,15> result{};
    for(const auto& row:farm.tiles)for(const auto& tile:row) {
        if(tile.kind==T_PLANT)++result[tile.what];
        else if(tile.has_animal)++result[tile.what];
        else if(tile.kind==T_PASTURE)++result[12];
        else if(tile.kind==T_COOP)++result[13];
        else if(tile.kind==T_WEED)++result[14];
    }
    return result;
}

int distance(const Observation& o) {
    const auto a=counts(o.self()),b=counts(o.opponent());
    int result=std::abs(o.self().n_units-o.opponent().n_units)+3*std::abs(o.self().n_quadrants-o.opponent().n_quadrants);
    for(int i=0;i<15;++i)result+=std::abs(a[i]-b[i]);
    return result;
}

double daily_demand(const Observation& o,const Configuration& cfg,int item,bool legacy) {
    double value=0;
    for(int s=0;s<o.n_shops;++s)if(SHOP_MASK[o.shops[s]]&(1<<item))
        value+=double(cfg.turns_per_day)/std::max(1,cfg.shop_sell_interval)*SHOP_MULT[o.shops[s]];
    if(item!=FERTILIZER)value+=double(cfg.turns_per_day)/std::max(1,cfg.center_sell_interval)*(legacy?(o.day>=20?4:o.day>=10?2:1):1);
    return value;
}

bool sale(const Order& order){return order.op==M_SELL && order.item<N_PRODUCTS;}
int legacy_quote(int item,int inventory) {
    if(inventory>=10000 || (item!=CARROT && item!=TOMATO && item!=EGG))return market_price(item,inventory);
    const double shortage=10000-inventory;
    double quote=item==CARROT?35+7*std::log1p(shortage)/std::log1p(450):
        item==TOMATO?60+24*shortage/200:50+20*shortage/332;
    return std::max(1,int(std::nearbyint(quote)));
}
double score(const Observation& o,const Configuration& cfg,const Order& order,double alpha,bool legacy=false) {
    const int item=order.item,q=std::max(0,order.n),inventory=o.market.inventory[item];
    double result=q*std::max(0,o.market.prices[item]-(legacy?legacy_quote(item,inventory+q):market_price(item,inventory+q)));
    if(result>0 && alpha>0) {
        const double excess=std::max(0,inventory+q-10000),demand=std::max(.25,daily_demand(o,cfg,item,false));
        result*=1+alpha*std::min(1.0,excess/demand/10);
    }
    return result;
}

void reorder(const Observation& o,const Configuration& cfg,Action& action) {
    struct Ranked {double score;int index;Order order;};
    std::array<Ranked,10> rows{};int n=0;
    const double alpha=cfg.center_sell_interval>=24?.25:0;
    for(int i=0;i<action.n_orders;++i)if(sale(action.orders[i]))rows[n++]={score(o,cfg,action.orders[i],alpha),i,action.orders[i]};
    std::sort(rows.begin(),rows.begin()+n,[](const auto& a,const auto& b){return a.score!=b.score?a.score>b.score:a.index<b.index;});
    for(int i=0,j=0;i<action.n_orders;++i)if(sale(action.orders[i]))action.orders[i]=rows[j++].order;
}

std::array<int,N_ITEMS> projected(const Observation& o,const Action& a) {
    std::array<int,N_ITEMS> shed{};std::copy_n(o.own.shed,N_ITEMS,shed.begin());
    int total=std::accumulate(shed.begin(),shed.end(),0);
    for(int u=0;u<a.n_units;++u) {
        int x=o.self().pos_x[u],y=o.self().pos_y[u];if(!is_shed_adjacent(x,y,BOARD))continue;
        auto deposit=[&](int item,int requested) {
            int n=std::min({std::max(0,requested),int(o.own.inv[u][item]),std::max(0,100-total)});
            shed[item]+=n;total+=n;
        };
        const auto command=a.units[u];const auto& tile=o.self().tiles[y][x];
        if(command.op==OP_DROP)for(int k=0;k<o.own.inv_nkeys[u];++k) {
            int item=o.own.inv_keys[u][k];deposit(item,o.own.inv[u][item]);
        } else if(command.op==OP_PLACE) {
            bool placing=is_animal(command.arg) && !tile.has_animal && tile.kind==(command.arg==GOOSE?T_COOP:T_PASTURE);
            if(!placing)deposit(command.arg,command.n);
        }
    }
    return shed;
}

void advance_finished(const Observation& o,const Configuration& cfg,Action& a) {
    constexpr int finished[]={MILK,WOOL,EGG,STRAWBERRY,MELON,TOMATO,CARROT};
    std::array<bool,N_PRODUCTS> eligible{},handled{};for(int item:finished)eligible[item]=true;
    auto shed=projected(o,a);
    for(int u=0;u<a.n_units;++u)if(a.units[u].op==OP_PICKUP && a.units[u].arg<N_PRODUCTS && eligible[a.units[u].arg])
        shed[a.units[u].arg]=std::max(0,shed[a.units[u].arg]-std::max(0,a.units[u].n));
    for(int m=0;m<a.n_orders;++m)if(sale(a.orders[m]) && eligible[a.orders[m].item]) {
        int item=a.orders[m].item;a.orders[m].n=handled[item]?0:shed[item];handled[item]=true;
    }
    struct Ranked {double score;int index,item,n;};std::array<Ranked,7> rows{};int n=0;
    for(int i=0;i<7;++i) {
        int item=finished[i];if(handled[item] || shed[item]<=0)continue;
        rows[n++]={score(o,cfg,{M_SELL,uint8_t(item),shed[item]},.25,true),i,item,shed[item]};
    }
    std::sort(rows.begin(),rows.begin()+n,[](const auto& a,const auto& b){return a.score!=b.score?a.score>b.score:a.index<b.index;});
    for(int i=0;i<n && a.n_orders<std::min(10,cfg.max_orders);++i)
        a.orders[a.n_orders++]={M_SELL,uint8_t(rows[i].item),rows[i].n};
}
}

void AgentCore::act(const Observation& o,const kag::agent::DecisionBudget&,Action& action) {
    if(o.step<0 || o.step>=719)std::abort();
    std::array<Action,3> actions{};
    for(int route=0;route<3;++route) {
        auto& a=actions[route];route_action(route,o.step,o.self().n_units,a);
        for(int u=0;u<MAX_UNITS;++u) {
            auto& repair=repairs_[route][u];int age=o.step-repair.start;
            if(u>=a.n_units){repair.start=-100;continue;}
            if(age==1)a.units[u]=repair.intended;
            else if(age>=2 && age<=9) {
                Action previous;route_action(route,o.step-1,a.n_units,previous);a.units[u]=previous.units[u];
            } else repair.start=-100;
        }
        for(int u=0;u<a.n_units;++u) {
            auto& repair=repairs_[route][u];auto& command=a.units[u];
            if(repair.start>=0 || (command.op!=OP_BUILD_PASTURE && command.op!=OP_PLANT))continue;
            if(o.self().tiles[o.self().pos_y[u]][o.self().pos_x[u]].kind==T_WEED){repair={o.step,command};command={OP_DIG,0,1};}
        }
        reorder(o,config_,a);
    }
    route_=0;
    if(o.n_shops && o.shops[0]==SHOP_YARN_STORE && o.step>=88)route_=1;
    else if(o.n_shops>=2 && o.shops[0]!=SHOP_YARN_STORE && o.shops[1]==SHOP_YARN_STORE && o.step>=153)route_=2;
    action=actions[route_];
    if(overlay_ && distance(o)<=2) {
        auto old=action;advance_finished(o,config_,action);
        bool changed=old.n_orders!=action.n_orders;
        for(int i=0;i<std::min(old.n_orders,action.n_orders);++i)
            changed|=old.orders[i].op!=action.orders[i].op || old.orders[i].item!=action.orders[i].item || old.orders[i].n!=action.orders[i].n;
        advanced_turns_+=changed;
    }
    action.finalize();
}
}
