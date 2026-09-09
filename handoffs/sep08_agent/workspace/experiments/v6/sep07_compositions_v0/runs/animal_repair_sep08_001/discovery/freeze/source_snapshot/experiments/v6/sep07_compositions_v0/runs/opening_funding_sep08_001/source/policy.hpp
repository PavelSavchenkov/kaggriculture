#pragma once
#include "experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
namespace compositions::opening_funding {
class Policy {
    empty_sale_slots_m2::Agent base_;
    int quantity_;
public:
    explicit Policy(int quantity):quantity_(quantity){if(quantity<0 || quantity>32)std::abort();}
    static kag::agent::AgentInfo info(){return {"opening_funding"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        base_.act(o,b,a);
        if(o.step==0){
            // This parent emits a buy/sell round trip followed by its farm's
            // wheat purchase. Keep the turn's ordering and all later actions.
            if(a.n_orders<2 || a.orders[0].op!=kag::M_BUY_PRODUCT || a.orders[1].op!=kag::M_SELL ||
               a.orders[0].item!=kag::WHEAT || a.orders[1].item!=kag::WHEAT)std::abort();
            a.orders[0].n=quantity_;a.orders[1].n=quantity_;a.finalize();
        }
    }
};
}
