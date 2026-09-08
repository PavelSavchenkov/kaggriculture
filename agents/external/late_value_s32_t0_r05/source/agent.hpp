#pragma once
#include "deps/runs/late_portfolio_001/proposals/late_value_s32_t0_r05/source/agent.hpp"
namespace kag::agents::late_value_s32_t0_r05 {
class Agent {
    ::kag::catalog_late_value_s32_t0_r05_agents::late_value_s32_t0_r05::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
