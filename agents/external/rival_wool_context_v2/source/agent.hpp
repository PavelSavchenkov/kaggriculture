#pragma once
#include "deps/runs/rival_wool_context_002/proposals/rival_wool_context_v2/source/agent.hpp"
namespace kag::agents::rival_wool_context_v2 {
class Agent {
    ::kag::catalog_rival_wool_context_v2_agents::rival_wool_context_v2::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
