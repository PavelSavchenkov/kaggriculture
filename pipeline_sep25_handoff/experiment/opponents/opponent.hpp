#pragma once
// In-house league agents as full-game opponents, each compiled in its own target
// with only its own include paths (their headers use paths that clash with ours).
#include "agents/common/api/agent_api.hpp"
#include <memory>

namespace opponents {
struct Opponent {
    virtual ~Opponent() = default;
    virtual void reset(const kag::agent::AgentInit& init) = 0;
    virtual void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) = 0;
    virtual std::unique_ptr<Opponent> clone() const = 0;  // for rollouts; nullptr if not copyable
};
std::unique_ptr<Opponent> make(const char* name);  // nullptr when unknown
}
