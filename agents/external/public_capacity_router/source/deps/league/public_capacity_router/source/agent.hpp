#pragma once
#include "../../../../../../../common/api/agent_api.hpp"
namespace catalog_public_capacity_router_compositions::public_capacity_router {
class AgentCore {
    int route_=0;
    bool terminal_;
public:
    explicit AgentCore(bool terminal=false):terminal_(terminal) {}
    static kag::agent::AgentInfo info() {return {"public_capacity_router"};}
    void reset(const kag::agent::AgentInit&) {route_=0;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
class Agent:public AgentCore {};
}
