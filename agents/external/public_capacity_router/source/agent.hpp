#pragma once
#include "deps/league/public_capacity_router/source/agent.hpp"
namespace kag::agents::public_capacity_router {
class Agent {
    ::catalog_public_capacity_router_compositions::public_capacity_router::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
