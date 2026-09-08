#pragma once
#include "deps/runs/late_animal_schedule_001/proposals/late_goose_wheat_context/source/agent.hpp"
namespace kag::agents::late_goose_wheat_context {
class Agent {
    ::kag::catalog_late_goose_wheat_context_agents::late_goose_wheat_context::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
