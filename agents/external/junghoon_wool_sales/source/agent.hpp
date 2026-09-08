#pragma once
#include "deps/candidates/junghoon_wool_sales/source/agent.hpp"
namespace kag::agents::junghoon_wool_sales {
class Agent {
    ::catalog_junghoon_wool_sales_compositions::junghoon_wool_sales::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
