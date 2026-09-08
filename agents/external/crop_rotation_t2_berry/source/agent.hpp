#pragma once
#include "deps/runs/crop_rotation_berry_gate_001/proposals/crop_rotation_t2_berry/source/agent.hpp"
namespace kag::agents::crop_rotation_t2_berry {
class Agent {
    ::catalog_crop_rotation_t2_berry_compositions::crop_rotation_t2_berry::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
