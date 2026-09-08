#pragma once
#include "deps/candidates/opening_router_v4/source/agent.hpp"
namespace kag::agents::opening_router_v4 {
class Agent {
    ::catalog_opening_router_v4_compositions::opening_router_v4::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
