#pragma once
#include "../../../league/public_router/source/agent.hpp"
#include "../../../league/feeltheagi_55/source/agent.hpp"

namespace compositions::opening_router_v0 {
class Agent {
public:
    static kag::agent::AgentInfo info() {return {"opening_router_v0"};}
    void reset(const kag::agent::AgentInit& init) {
        public_.reset(init);teacher_.reset(init);teacher_selected_=false;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        // Both courses have the same step-zero action. Choose before their
        // first different market order, using only the now-visible opponent.
        if(o.step==1)teacher_selected_=o.farms[o.player^1].n_units==1;
        if(teacher_selected_)teacher_.act(o,budget,action);else public_.act(o,budget,action);
    }
private:
    public_router::Agent public_;
    feeltheagi_55::Agent teacher_;
    bool teacher_selected_=false;
};
}
