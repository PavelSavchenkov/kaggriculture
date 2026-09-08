#pragma once
#include "deps/runs/salem_port_sep08_001/proposals/salem_sep08_m3/source/agent.hpp"
namespace kag::agents::salem_sep08_m3 {
class Agent {
    ::catalog_salem_sep08_m3_compositions::salem_sep08_m3::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
