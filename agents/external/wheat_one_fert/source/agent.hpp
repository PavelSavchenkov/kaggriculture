#pragma once
#include "deps/runs/productive_wheat_rotation_001/proposals/wheat_one_fert/source/agent.hpp"
namespace kag::agents::wheat_one_fert {
class Agent {
    ::catalog_wheat_one_fert_compositions::wheat_one_fert::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
