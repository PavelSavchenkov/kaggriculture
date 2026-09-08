#pragma once
#include "agents/common/api/agent_api.hpp"

namespace kag::agents::public_router_v52 {
class Agent {
    int last_step_=-1,last_block_=-1,route_=0;
public:
    static kag::agent::AgentInfo info(){return {"public_router_v52"};}
    void reset(const kag::agent::AgentInit&){last_step_=last_block_=-1;route_=0;}
    int selected_route()const{return route_;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
