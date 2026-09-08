#pragma once
#include "deps/runs/opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp"
namespace kag::agents::opening_q32_b13_v1 {
class Agent {
    ::catalog_opening_q32_b13_v1_compositions::opening_q32_b13_v1::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
