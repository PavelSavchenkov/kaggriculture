#pragma once
#include "deps/runs/yusuke_port_sep08_001/proposals/yusuke_sep08_m2/source/agent.hpp"
namespace kag::agents::yusuke_sep08_m2 {
class Agent {
    ::catalog_yusuke_sep08_m2_compositions::yusuke_sep08_m2::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
