#pragma once
#include "deps/league/public_router_v52/source/agent.hpp"
namespace kag::agents::public_router_v52 {
class Agent {
    ::kag::catalog_public_router_v52_agents::public_router_v52::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
