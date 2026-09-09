#pragma once
#include "deps/runs/animal_service_policy_sep08_002/proposals/cow_service_retained_q24_premium_m2/source/agent.hpp"
namespace kag::agents::early_structure_cow_parent {
class Agent {
    ::catalog_early_structure_cow_parent_compositions::cow_service_retained_q24_premium_m2::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
