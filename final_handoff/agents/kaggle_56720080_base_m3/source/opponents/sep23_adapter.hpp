#pragma once
// agent_sep23 behind an opaque handle: its headers use "source/..." paths that clash
// with this experiment's, so it is compiled in a separate target.
#include "agents/common/api/agent_api.hpp"
#include <memory>

namespace sep23 {
class Opponent {
public:
    Opponent();
    ~Opponent();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation, const kag::agent::DecisionBudget& budget, kag::Action& action);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
