#pragma once
#include "deps/runs/crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp"
namespace kag::agents::crop_value_m2_t4 {
class Agent {
    ::catalog_crop_value_m2_t4_compositions::crop_value_m2_t4::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
