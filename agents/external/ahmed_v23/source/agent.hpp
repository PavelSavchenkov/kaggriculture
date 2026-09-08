#pragma once
#include "deps/league/ahmed_v23/source/agent.hpp"
namespace kag::agents::ahmed_v23 {
class Agent {
    ::kag::catalog_ahmed_v23_agents::ahmed_v23::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
