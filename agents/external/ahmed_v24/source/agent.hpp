#pragma once
#include "deps/runs/premium_sales_sep08_001/proposals/ahmed_v24/source/agent.hpp"
namespace kag::agents::ahmed_v24 {
class Agent {
    ::catalog_ahmed_v24_compositions::ahmed_v24::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
