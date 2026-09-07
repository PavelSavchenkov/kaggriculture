#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::public_router_v5 {
class Agent {
    int last_step_=-1, route_=0;
public:
    static kag::agent::AgentInfo info() {return {"public_router_v5"};}
    void reset(const kag::agent::AgentInit&) {last_step_=-1;route_=0;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
