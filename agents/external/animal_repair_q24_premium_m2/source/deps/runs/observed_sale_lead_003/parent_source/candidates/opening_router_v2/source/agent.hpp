#pragma once
#include "../../../league/public_router/source/agent.hpp"
#include "../../firstday_m85_sale7/source/agent.hpp"
namespace catalog_animal_repair_q24_premium_m2_sale::opening_router_v2 {
class Agent {
    public_router::Agent public_;
    firstday_m85_sale7::Agent teacher_;
    bool teacher_selected_=false;
public:
    static kag::agent::AgentInfo info() {return {"opening_router_v2"};}
    void reset(const kag::agent::AgentInit& init) {
        public_.reset(init);teacher_.reset(init);teacher_selected_=false;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        if(o.step==1)teacher_selected_=o.opponent().n_units==1;
        if(teacher_selected_)teacher_.act(o,budget,action);else public_.act(o,budget,action);
    }
};
}
