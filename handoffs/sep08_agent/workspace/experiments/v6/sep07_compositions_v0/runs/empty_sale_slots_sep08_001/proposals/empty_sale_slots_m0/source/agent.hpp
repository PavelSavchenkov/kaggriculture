#pragma once
#include "../../../../observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
namespace compositions::empty_sale_slots_m0 {
class Agent:public kag::agents::observed_sale_lead_start_216::Agent {
    int removed_=0;
public:
    static kag::agent::AgentInfo info() {return {"empty_sale_slots_m0"};}
    void reset(const kag::agent::AgentInit& init) {kag::agents::observed_sale_lead_start_216::Agent::reset(init);removed_=0;}
    int removed_slots()const{return removed_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        kag::agents::observed_sale_lead_start_216::Agent::act(o,b,a);
        int kept=0;
        for(int i=0;i<a.n_orders;++i) {
            const auto order=a.orders[i];
            bool eligible=0==1 || (0==2 && o.step>=216 && order.item>kag::WHEAT && order.item<kag::FERTILIZER);
            if(eligible && order.op==kag::M_SELL && order.n<=0){++removed_;continue;}
            a.orders[kept++]=order;
        }
        a.n_orders=kept;a.finalize();
    }
};
}
