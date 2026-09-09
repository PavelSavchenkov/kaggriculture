#pragma once
#include "agents/common/api/agent_api.hpp"
#include <algorithm>
#include <array>

namespace compositions::salem_port {
const kag::Action& planned(int step);

class Policy {
    struct Repair {int start=-1000;kag::UnitAction intended{};bool active=false;};
    std::array<Repair,kag::MAX_UNITS> repairs_{};
    std::array<int,kag::N_PRODUCTS> due_{};
    int mode_=3,last_=-1,due_step_=-1;
    void weed(const kag::agent::AgentObservation& o,kag::Action& a){
        for(int u=0;u<kag::MAX_UNITS;++u){
            auto& r=repairs_[u];if(!r.active)continue;
            if(u>=a.n_units){r.active=false;continue;}
            const int age=o.step-r.start;
            if(age==1)a.units[u]=r.intended;
            else if(age>=2 && age<=9){const auto& previous=planned(o.step-1);a.units[u]=u<previous.n_units?previous.units[u]:kag::UnitAction{};}
            else r.active=false;
        }
        for(int u=0;u<a.n_units;++u){
            auto& r=repairs_[u];const auto action=a.units[u];
            if(r.active || (action.op!=kag::OP_BUILD_PASTURE && action.op!=kag::OP_PLANT))continue;
            const auto& farm=o.self();
            if(farm.tiles[farm.pos_y[u]][farm.pos_x[u]].kind!=kag::T_WEED)continue;
            r={o.step,action,true};a.units[u]={kag::OP_DIG,0,1};
        }
    }
    static bool demand(const kag::agent::AgentObservation& o,int item){
        if(o.step%24==0)return true;
        if(o.step%4!=0)return false;
        for(int j=0;j<o.n_shops;++j)if(kag::SHOP_MASK[o.shops[j]]&(1u<<item))return true;
        return false;
    }
    void sales(const kag::agent::AgentObservation& o,kag::Action& a){
        if(due_step_>=0 && due_step_<o.step){due_step_=-1;due_.fill(0);}
        if(due_step_==o.step){
            int kept=0;
            for(int j=0;j<a.n_orders;++j){
                auto order=a.orders[j];
                if(order.op==kag::M_SELL && order.item<kag::N_PRODUCTS && due_[order.item]>0){
                    const int removed=std::min(std::max(0,order.n),due_[order.item]);
                    order.n=std::max(0,order.n)-removed;due_[order.item]-=removed;
                    if(order.n<=0)continue;
                }
                a.orders[kept++]=order;
            }
            a.n_orders=kept;due_step_=-1;due_.fill(0);
        }
        if(o.step+1>=720)return;
        constexpr int items[]={kag::MELON,kag::MILK,kag::STRAWBERRY,kag::WOOL};
        for(int item:items){
            if(demand(o,item))continue;
            int target=0,reserve=0,at=-1;
            const auto& next=planned(o.step+1);
            for(int j=0;j<next.n_orders;++j)if(next.orders[j].op==kag::M_SELL && next.orders[j].item==item)target+=std::max(0,next.orders[j].n);
            for(int u=0;u<a.n_units;++u)if(a.units[u].op==kag::OP_PICKUP && a.units[u].arg==item)reserve+=std::max(0,a.units[u].n);
            for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==kag::M_SELL && a.orders[j].item==item){
                reserve+=std::max(0,a.orders[j].n);if(at<0)at=j;
            }
            const int quantity=std::min(target,std::max(0,int(o.own.shed[item])-reserve));
            if(quantity<=0)continue;
            if(at>=0)a.orders[at].n=std::max(0,a.orders[at].n)+quantity;
            else if(a.n_orders<10)a.orders[a.n_orders++]={kag::M_SELL,uint8_t(item),quantity};
            else continue;
            due_[item]+=quantity;due_step_=o.step+1;
        }
    }
public:
    explicit Policy(int mode):mode_(mode){}
    void reset(const kag::agent::AgentInit&){repairs_.fill({});due_.fill(0);last_=due_step_=-1;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& a){
        if(o.step<=0 || o.step<last_){repairs_.fill({});due_.fill(0);due_step_=-1;}
        last_=o.step;a=planned(o.step);
        const int old=a.n_units;a.n_units=o.self().n_units;
        for(int u=old;u<a.n_units;++u)a.units[u]={};
        if(mode_&1)weed(o,a);
        if(mode_&2)sales(o,a);
        a.finalize();
    }
};
}
