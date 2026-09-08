#pragma once
#include "deps/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
namespace kag::agents::shop_herd_s6_m3_g1 {
class Agent {
    ::catalog_shop_herd_s6_m3_g1_compositions::shop_herd_s6_m3_g1::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
