#pragma once
#include "../../../../crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"
namespace kag::agents::bohann_opening_v1::detail::bohann_opening_v1 {
// Bohann Wang, episode106497007 seat0: exact first two market sequences.
// The remainder, including all worker actions and adaptive branches, is local current.
class Agent {
    crop_mix_t2_wheat::Agent base_;
public:
    static kag::agent::AgentInfo info(){return {"bohann_opening_v1"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        base_.act(o,b,a);
        using namespace kag;
        if(o.step==0){
            a.n_orders=3;
            a.orders[0]={M_BUY_PRODUCT,WHEAT,81};
            a.orders[1]={M_SELL,WHEAT,81};
            a.orders[2]={M_BUY_PRODUCT,WHEAT,13};
        }else if(o.step==1){
            a.n_orders=8;
            a.orders[0]={M_SELL,WHEAT,8};
            for(int i=1;i<=5;++i)a.orders[i]={M_HIRE,0,0};
            a.orders[6]={M_BUY_ANIMAL,COW,2};
            a.orders[7]={M_BUY_ANIMAL,SHEEP,2};
        }
        a.finalize();
    }
};
}
