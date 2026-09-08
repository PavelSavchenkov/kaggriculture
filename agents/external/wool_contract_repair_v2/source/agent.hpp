#pragma once
#include "deps/runs/wool_contract_repair_002/proposals/wool_contract_repair_v2/source/agent.hpp"
namespace kag::agents::wool_contract_repair_v2 {
class Agent {
    ::kag::catalog_wool_contract_repair_v2_agents::wool_contract_repair_v2::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
