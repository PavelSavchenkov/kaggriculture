#pragma once
#include "deps/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
namespace kag::agents::observed_sale_lead_start_216 {
class Agent {
    ::kag::catalog_observed_sale_lead_start_216_agents::observed_sale_lead_start_216::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
