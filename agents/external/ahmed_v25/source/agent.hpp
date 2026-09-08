#pragma once
#include "deps/runs/ahmed_v25_sep08_001/proposals/ahmed_v25/source/agent.hpp"
namespace kag::agents::ahmed_v25 {
class Agent {
    ::catalog_ahmed_v25_compositions::ahmed_v25::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
