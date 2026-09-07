#include "agent.hpp"
#include <algorithm>
#include <cmath>

namespace compositions::boatlee_v29 {
using namespace kag;
using Obs=agent::AgentObservation;
#include "data.inc"
constexpr int items[]={STRAWBERRY,MILK,WOOL},base_prices[]={120,160,200};

Action source_action(int step) {
    const int* p=data+offsets[std::clamp(step,0,int(std::size(offsets))-1)];Action a;
    a.n_units=*p++;a.n_orders=*p++;
    for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
    for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
    a.finalize();return a;
}

int shop_demand(const uint8_t* shops,int count,int item) {
    int n=0;for(int i=0;i<count;++i)if(SHOP_MASK[shops[i]]&(1u<<item))n+=SHOP_MULT[shops[i]];
    return n;
}
int sold(const Action& a,int item) {
    int n=0;for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==M_SELL && a.orders[i].item==item)n+=std::max(0,a.orders[i].n);
    return n;
}
int pickup(const Action& a,int item) {
    int n=0;for(int u=0;u<a.n_units;++u)if(a.units[u].op==OP_PICKUP && a.units[u].arg==item)n+=std::max(0,a.units[u].n);
    return n;
}
template<class Farm> int count(const Farm& farm,int item) {
    int result=0;
    for(const auto& row:farm.tiles)for(const auto& t:row)
        result+=item==STRAWBERRY?(t.kind==T_PLANT && t.what==item):(t.has_animal && t.what==item);
    return result;
}

void Agent::reset(const agent::AgentInit&) {
    weed_start_.fill(-1);weed_intended_.fill({});last_inventory_.fill(0);last_sold_.fill(0);
    added_.fill(0);pressure_.fill(0);last_shops_.fill(0);last_step_=-1;last_shop_count_=0;mirror_=-1;
}

void Agent::weed(const Obs& o,Action& a) {
    const auto previous=source_action(o.step-1);
    for(int u=0;u<MAX_UNITS;++u)if(weed_start_[u]>=0) {
        if(u>=a.n_units){weed_start_[u]=-1;continue;}
        int age=o.step-weed_start_[u];
        if(age==1)a.units[u]=weed_intended_[u];
        else if(age>=2 && age<=9)a.units[u]=u<previous.n_units?previous.units[u]:UnitAction{};
        else weed_start_[u]=-1;
    }
    for(int u=0;u<a.n_units;++u) {
        const auto q=a.units[u];
        if(weed_start_[u]>=0 || (q.op!=OP_BUILD_PASTURE && q.op!=OP_PLANT))continue;
        const auto& t=o.self().tiles[o.self().pos_y[u]][o.self().pos_x[u]];
        if(t.kind!=T_WEED)continue;
        weed_start_[u]=o.step;weed_intended_[u]=q;a.units[u]={OP_DIG,0,1};
    }
}

void Agent::market(const Obs& o,Action& a) {
    const int step=o.step;
    if(step==0 || step<=last_step_) {
        last_step_=step;last_inventory_.fill(0);last_sold_.fill(0);added_.fill(0);
        pressure_.fill(0);last_shop_count_=0;mirror_=-1;
    }
    if(last_step_==step-1)for(int k=0;k<3;++k) {
        const int item=items[k];
        const int demand=((step-1)%24==0)+((step-1)%4==0?shop_demand(last_shops_.data(),last_shop_count_,item):0);
        const int external=o.market.inventory[item]-last_inventory_[k]+demand-last_sold_[k];
        pressure_[k]=std::max(0.,pressure_[k]*.72+std::min(24.,std::max(0.,double(external))));
    }
    last_step_=step;
    if(mirror_<0 && step>=config::mirror_latch_step) {
        int distance=0;for(int item:{int(COW),int(SHEEP),int(STRAWBERRY)})distance+=std::abs(count(o.self(),item)-count(o.opponent(),item));
        mirror_=distance<=config::mirror_composition_distance && o.self().n_units==o.opponent().n_units &&
            o.self().n_quadrants==o.opponent().n_quadrants && std::abs(o.self().money-o.opponent().money)<=config::mirror_money_distance;
    }
    const int total=o.own.shed_total;
    if(step>=config::start_step)for(int k=0;k<3;++k) {
        const int item=items[k],stock=o.own.shed[item],take=pickup(a,item);
        const int scheduled=std::min(std::max(0,stock-take),sold(a,item));
        const int unscheduled=std::max(0,stock-take-scheduled);
        int reserve=step<528?config::reserve_early:step<600?config::reserve_mid:step<648?config::reserve_late:
            step<684?config::reserve_tail:step<704?2:0;
        if(total>=config::shed_hard)reserve=0;
        else if(total>=config::shed_soft)reserve=std::max(0,reserve-config::shed_reserve_cut);
        const int demand=shop_demand(o.shops,o.n_shops,item);
        if(step<684)reserve+=std::min(config::demand_reserve_cap,demand*config::demand_reserve);
        const int excess=std::max(0,unscheduled-reserve);if(!excess)continue;
        const int maximum=mirror_==1?config::mirror_max_extra_per_item:config::max_extra_per_item;
        const int budget=std::max(0,maximum-added_[k]);if(!budget)continue;
        const double ratio=double(o.market.prices[item])/base_prices[k];
        const int kind=item==STRAWBERRY?STRAWBERRY:item==MILK?COW:SHEEP;
        const int capacity=count(o.opponent(),kind)-count(o.self(),kind);
        double gate=config::price_gate;
        if(step<684)gate+=std::min(config::demand_gate_cap,demand*config::demand_gate);
        if(step>=648)gate-=.12;if(step>=684)gate-=.18;
        if(pressure_[k]>=config::pressure_trigger)gate-=config::pressure_gate_cut;
        if(capacity>=config::capacity_trigger)gate-=config::capacity_gate_cut;
        const bool urgent=total>=config::shed_hard || step>=704;
        if(!urgent && ratio<std::max(.20,gate))continue;
        int tranche=config::tranche;
        if(pressure_[k]>=config::pressure_trigger)tranche+=config::pressure_tranche;
        if(total>=config::shed_soft)tranche+=config::shed_tranche;
        if(step>=684)tranche+=config::tail_tranche;
        const int quantity=std::min({excess,std::max(1,tranche),budget});
        int found=-1;for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==M_SELL && a.orders[i].item==item){found=i;break;}
        if(found>=0)a.orders[found].n=std::max(0,a.orders[found].n)+quantity;
        else if(a.n_orders<10) {
            for(int i=a.n_orders;i>0;--i)a.orders[i]=a.orders[i-1];
            ++a.n_orders;a.orders[0]={M_SELL,uint8_t(item),quantity};
        }
        // Literal source accounting charges the intended quantity even when a
        // full market queue prevented insertion. It is not exact sale feedback.
        added_[k]+=quantity;
    }
    for(int k=0;k<3;++k){last_inventory_[k]=o.market.inventory[items[k]];last_sold_[k]=std::min(int(o.own.shed[items[k]]),sold(a,items[k]));}
    last_shop_count_=o.n_shops;std::copy_n(o.shops,o.n_shops,last_shops_.begin());
}

void Agent::act(const Obs& o,const agent::DecisionBudget&,Action& a) {
    a=source_action(o.step);
    for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};
    a.n_units=o.self().n_units;
    weed(o,a);market(o,a);a.finalize();
}
}
