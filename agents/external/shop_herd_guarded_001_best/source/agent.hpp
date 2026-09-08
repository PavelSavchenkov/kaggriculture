#pragma once
#include "deps/candidates/shop_herd_guarded_001_best/source/agent.hpp"
namespace kag::agents::shop_herd_guarded_001_best {
class Agent {
    ::catalog_shop_herd_guarded_001_best_compositions::shop_herd_guarded_001_best::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
