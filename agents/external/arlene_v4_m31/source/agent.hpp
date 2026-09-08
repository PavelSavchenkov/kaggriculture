#pragma once
#include "deps/runs/arlene_v4_sep08_001/proposals/arlene_v4_m31/source/agent.hpp"
namespace kag::agents::arlene_v4_m31 {
class Agent {
    ::catalog_arlene_v4_m31_compositions::arlene_v4_m31::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
