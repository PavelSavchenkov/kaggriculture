#pragma once
#include "deps/runs/early_melon_sep08_001/proposals/early_melon_b98_m1/source/agent.hpp"
namespace kag::agents::early_melon_b98_m1 {
class Agent {
    ::catalog_early_melon_b98_m1_compositions::early_melon_b98_m1::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
