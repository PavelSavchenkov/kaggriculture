#pragma once
#include "deps/runs/empty_sale_floor_sep08_001/proposals/empty_sale_floor_m1/source/agent.hpp"
namespace kag::agents::empty_sale_floor_m1 {
class Agent {
    ::catalog_empty_sale_floor_m1_compositions::empty_sale_floor_m1::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
