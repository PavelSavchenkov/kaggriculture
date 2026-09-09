#pragma once
#include "agents/common/api/agent_api.hpp"
namespace compositions::arlene_v4_sep08 {
class AgentCore {
    int route_=0;
    int mask_;
public:
    explicit AgentCore(int mask=7):mask_(mask) {}
    static kag::agent::AgentInfo info() {return {"public_capacity_router"};}
    void reset(const kag::agent::AgentInit&) {route_=0;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
class Agent:public AgentCore {};
}
