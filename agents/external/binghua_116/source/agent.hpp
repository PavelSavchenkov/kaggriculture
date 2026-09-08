#pragma once
#include "deps/league/binghua_116/source/agent.hpp"
namespace kag::agents::binghua_116 {
class Agent {
    ::catalog_binghua_116_compositions::binghua_116::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
