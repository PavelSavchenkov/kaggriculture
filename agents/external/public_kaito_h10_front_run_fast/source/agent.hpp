#pragma once

#include <cstdint>

#include "agents/common/api/agent_api.hpp"
#include "base/agent.hpp"

namespace league::public_optimized::kaito_h10_front_run_fast {

struct SanitizerCounters {
    uint64_t fast_path = 0;
    uint64_t fallback = 0;
};

class Policy {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

    const SanitizerCounters& sanitizer_counters() const { return counters_; }

private:
    league::public_unchanged::kaito::Policy source_{};
    kag::agent::AgentConfig config_{};
    SanitizerCounters counters_{};
};

}
