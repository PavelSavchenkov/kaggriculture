#pragma once
#include "agents/common/api/agent_api.hpp"
namespace catalog_early_structure_cow_parent_compositions::animal_repair {
template<class Parent,int Quantity> class Opening:public Parent {
public:
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        Parent::act(o,b,a);
        if(o.step==0){
            if(a.n_orders<2 || a.orders[0].op!=kag::M_BUY_PRODUCT || a.orders[1].op!=kag::M_SELL ||
                a.orders[0].item!=kag::WHEAT || a.orders[1].item!=kag::WHEAT)std::abort();
            a.orders[0].n=Quantity;a.orders[1].n=Quantity;a.finalize();
        }
    }
};
}
