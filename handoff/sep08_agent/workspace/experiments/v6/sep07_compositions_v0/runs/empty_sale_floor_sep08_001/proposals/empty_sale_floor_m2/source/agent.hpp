#pragma once
#include "../../../../observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
namespace compositions::empty_sale_floor_m2 {
class Agent:public kag::agents::observed_sale_lead_start_216::Agent {
    int removed_=0,protected_=0;
public:
    static kag::agent::AgentInfo info() {return {"empty_sale_floor_m2"};}
    void reset(const kag::agent::AgentInit& init) {
        kag::agents::observed_sale_lead_start_216::Agent::reset(init);removed_=protected_=0;
    }
    int removed_slots()const{return removed_;}
    int protected_slots()const{return protected_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        kag::agents::observed_sale_lead_start_216::Agent::act(o,b,a);
        if(o.step<216)return;
        bool unsafe[10]{};
        if constexpr(2>0) {
            std::array<int,kag::N_PRODUCTS> inventory;
            std::copy_n(o.market.inventory,kag::N_PRODUCTS,inventory.begin());
            for(int i=0;i<a.n_orders;++i) {
                const auto& order=a.orders[i];
                if(order.op==kag::M_BUY_PRODUCT && order.n>0)inventory[order.item]-=order.n;
                if(order.op!=kag::M_SELL || order.n<=0)continue;
                inventory[order.item]+=order.n;
                const int rival_reserve=2==2?int(order.n):(2==3?100:0);
                unsafe[i]=kag::market_price(order.item,inventory[order.item]+rival_reserve)<=1;
            }
        }
        int kept=0;
        for(int i=0;i<a.n_orders;++i) {
            const auto order=a.orders[i];
            const bool eligible=order.op==kag::M_SELL && order.n<=0 && order.item>kag::WHEAT && order.item<kag::FERTILIZER;
            bool protected_slot=false;
            if(eligible)for(int j=i+1;j<a.n_orders;++j)protected_slot|=unsafe[j];
            if(eligible && !protected_slot){++removed_;continue;}
            protected_+=protected_slot;
            a.orders[kept++]=order;
        }
        a.n_orders=kept;a.finalize();
    }
};
}
