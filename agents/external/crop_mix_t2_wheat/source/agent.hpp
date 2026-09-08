#pragma once
#include "deps/runs/crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"
namespace kag::agents::crop_mix_t2_wheat {
class Agent {
    ::catalog_crop_mix_t2_wheat_compositions::crop_mix_t2_wheat::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
