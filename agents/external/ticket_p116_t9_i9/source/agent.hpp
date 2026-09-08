#pragma once
#include "deps/runs/animal_tickets_market_p116_v4_001/proposals/ticket_p116_t9_i9/source/agent.hpp"
namespace kag::agents::ticket_p116_t9_i9 {
class Agent {
    ::catalog_ticket_p116_t9_i9_compositions::ticket_p116_t9_i9::Agent policy_;
public:
    Agent() {}
    static kag::agent::AgentInfo info() { return decltype(policy_)::info(); }
    void reset(const kag::agent::AgentInit& init) { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { policy_.act(o, b, a); }
};
}
