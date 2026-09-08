#pragma once
#include "deps/runs/rival_wool_context_001/proposals/rival_wool_context_v1/source/agent.hpp"
namespace kag::agents::rival_wool_context_v1 {
class Agent {
    ::kag::catalog_rival_wool_context_v1_agents::rival_wool_context_v1::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
