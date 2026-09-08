#pragma once
#include "../../late_goose_optimized/source/agent.hpp"

namespace kag::catalog_late_goose_wheat_context_agents::late_goose_wheat_context {
class Agent {
    catalog_late_goose_wheat_context_compositions::opening_q32_b13_v1::Agent base_;
    kag::catalog_late_goose_wheat_context_agents::late_goose_optimized::Agent goose_;
    bool wheat_context_=false;
public:
    static kag::agent::AgentInfo info(){return {"late_goose_wheat_context"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);goose_.reset(init);wheat_context_=false;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        kag::Action alternative;base_.act(o,budget,action);goose_.act(o,budget,alternative);
        if(o.day==12 && o.hour==0) {
            int tomato_demand=0;
            for(int i=0;i<o.n_shops;++i)tomato_demand+=bool(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::TOMATO));
            wheat_context_=tomato_demand<2;
        }
        if(wheat_context_ && o.day>=13)action=alternative;
    }
};
}
