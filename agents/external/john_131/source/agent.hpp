#pragma once
#include "deps/league/john_131/source/agent.hpp"
namespace kag::agents::john_131 {
class Agent {
    ::catalog_john_131_compositions::john_131::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
