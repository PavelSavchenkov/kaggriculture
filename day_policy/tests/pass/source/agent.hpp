#pragma once
#include "agents/common/api/agent_api.hpp"
namespace kag::agents::day_policy_test_pass {
class Agent {
public:
    static agent::AgentInfo info() { return {"pass"}; }
    void reset(const agent::AgentInit&) {}
    void act(const agent::AgentObservation& o, const agent::DecisionBudget&, Action& a) {
        a.clear(); a.n_units = o.self().n_units;
        for (int u = 0; u < a.n_units; ++u) a.units[u] = {};
        a.finalize();
    }
};
static_assert(agent::LocalAgent<Agent>);
}
