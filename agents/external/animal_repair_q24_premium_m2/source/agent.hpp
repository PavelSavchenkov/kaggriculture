#pragma once
#include "deps/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/source/agent.hpp"
namespace kag::agents::animal_repair_q24_premium_m2 {
class Agent {
    ::catalog_animal_repair_q24_premium_m2_compositions::animal_repair_q24_premium_m2::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
