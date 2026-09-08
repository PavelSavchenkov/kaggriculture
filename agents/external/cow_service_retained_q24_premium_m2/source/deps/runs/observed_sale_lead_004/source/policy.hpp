#pragma once
#include "../../observed_sale_lead_003/parent_source/runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"
#include <array>

namespace catalog_cow_service_retained_q24_premium_m2_compositions::observed_sale_lead_after_funding {
using Base=kag::catalog_cow_service_retained_q24_premium_m2_sale_agents::rival_wool_context_v3::Agent;
using Obs=kag::agent::AgentObservation;
using Stock=std::array<int,kag::N_ITEMS>;
struct Diagnostics {int forecasts=0,matched=0,orders=0,units=0;};
class Policy {
    Base base_;
    int start_=24,mode_=0,due_=-1,forecast_due_=-1;
    Stock suppression_{},forecast_{};
    Diagnostics diagnostics_{};
    static Stock sales(const kag::Action& a){
        Stock s{};for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL)s[a.orders[i].item]+=std::max(0,int(a.orders[i].n));return s;
    }
    // Literal projection from Ahmed V23 (yhay81/tetsutani lineage): sequential
    // own unit deposits and withdrawals, including carried insertion order.
    static Stock projected(const Obs& o,const kag::Action& a){
        Stock s{};for(int i=0;i<kag::N_ITEMS;++i)s[i]=o.own.shed[i];int total=o.own.shed_total;
        for(int u=0;u<a.n_units;++u){
            int x=o.self().pos_x[u],y=o.self().pos_y[u];if((x!=4 && x!=5)||(y!=4 && y!=5))continue;
            const auto& v=a.units[u];
            if(v.op==kag::OP_PICKUP){int n=std::min(s[v.arg],std::max(0,int(v.n)));s[v.arg]-=n;total-=n;}
            else if(v.op==kag::OP_DROP){
                for(int k=0;k<o.own.inv_nkeys[u];++k){int i=o.own.inv_keys[u][k];int n=std::min(int(o.own.inv[u][i]),std::max(0,100-total));s[i]+=n;total+=n;}
            }else if(v.op==kag::OP_PLACE && v.arg<kag::GOOSE){int n=std::min({std::max(0,int(v.n)),int(o.own.inv[u][v.arg]),std::max(0,100-total)});s[v.arg]+=n;total+=n;}
        }
        return s;
    }
public:
    explicit Policy(int mode=1,int start=24):start_(start),mode_(mode){}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);due_=forecast_due_=-1;suppression_.fill(0);forecast_.fill(0);diagnostics_={};}
    const Diagnostics& diagnostics()const{return diagnostics_;}
    void act(const Obs& o,const kag::agent::DecisionBudget& budget,kag::Action& a){
        base_.act(o,budget,a);
        if(forecast_due_==o.step){++diagnostics_.forecasts;diagnostics_.matched+=sales(a)==forecast_;}
        if(due_==o.step)for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==kag::M_SELL){int removed=std::min(std::max(0,int(m.n)),suppression_[m.item]);m.n-=removed;suppression_[m.item]-=removed;}}
        due_=forecast_due_=-1;suppression_.fill(0);
        // Avoid shop/day entry guards and consumption between the current and
        // following turn. This is a forecast, not knowledge of future actions.
        if(o.step<start_ || o.step>=718 || o.hour==23 || o.step%4==0){a.finalize();return;}
        Stock available=projected(o,a);bool already[kag::N_ITEMS]{};
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL)already[a.orders[i].item]=true;
        bool possible=false;
        for(int item=1;item<kag::FERTILIZER;++item)
            possible|=available[item]>0 && !already[item] && o.market.prices[item]>=2 &&
                (mode_!=2 || item==kag::MILK || item==kag::WOOL);
        if(a.n_orders>=10 || !possible){a.finalize();return;}
        Base shadow=base_;Obs next=o;++next.step;next.hour=next.step%24;next.day=next.step/24;
        kag::Action predicted;shadow.act(next,budget,predicted);forecast_=sales(predicted);forecast_due_=o.step+1;
        if(!mode_){a.finalize();return;}
        for(int item=1;item<kag::FERTILIZER;++item){
            if(mode_==2 && item!=kag::MILK && item!=kag::WOOL)continue;
            if(already[item] || o.market.prices[item]<2)continue;
            int qty=std::min(available[item],forecast_[item]);if(qty<=0)continue;
            if(a.n_orders>=10)break;
            a.orders[a.n_orders++]={kag::M_SELL,uint8_t(item),qty};available[item]-=qty;
            suppression_[item]+=qty;due_=o.step+1;++diagnostics_.orders;diagnostics_.units+=qty;
        }
        a.finalize();
    }
};
}
