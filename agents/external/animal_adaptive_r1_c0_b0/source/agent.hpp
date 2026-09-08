#pragma once
#include "deps/runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
namespace kag::agents::animal_adaptive_r1_c0_b0 {
class Agent {
    ::catalog_animal_adaptive_r1_c0_b0_compositions::animal_adaptive_r1_c0_b0::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
