#pragma once
#include "../../../league/public_router/source/agent.hpp"
#include "../../../runs/advance_sales_001/proposals/advance_sales_001_7/source/agent.hpp"

namespace compositions::opening_router_v1 {
class Agent {
public:
    static kag::agent::AgentInfo info() {return {"opening_router_v1"};}
    void reset(const kag::agent::AgentInit& init) {
        public_.reset(init);teacher_.reset(init);teacher_selected_=false;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        if(o.step==1)teacher_selected_=o.opponent().n_units==1;
        if(teacher_selected_)teacher_.act(o,budget,action);else public_.act(o,budget,action);
    }
private:
    public_router::Agent public_;
    advance_sales_001_7::Agent teacher_;
    bool teacher_selected_=false;
};
}
