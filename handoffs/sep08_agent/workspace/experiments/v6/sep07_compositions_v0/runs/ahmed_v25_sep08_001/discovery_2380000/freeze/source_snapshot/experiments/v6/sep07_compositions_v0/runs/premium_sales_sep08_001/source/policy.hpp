#pragma once
#include "agents/common/api/agent_api.hpp"
namespace compositions::premium_sales {
inline void prioritize(kag::Action& action){
    kag::Order orders[10];int count=0;
    auto premium=[](const auto& m){return m.op==kag::M_SELL && m.item!=kag::WHEAT && m.item!=kag::FERTILIZER && m.n>0;};
    for(int i=0;i<action.n_orders;++i)if(premium(action.orders[i]))orders[count++]=action.orders[i];
    for(int i=0;i<action.n_orders;++i)if(!premium(action.orders[i]))orders[count++]=action.orders[i];
    for(int i=0;i<count;++i)action.orders[i]=orders[i];
    action.finalize();
}
template<class Parent,int First>
class Policy:public Parent {
public:
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action){
        Parent::act(o,budget,action);if(o.step>=First)prioritize(action);
    }
};
}
