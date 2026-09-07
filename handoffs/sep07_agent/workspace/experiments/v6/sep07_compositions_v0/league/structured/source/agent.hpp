#pragma once

#include "agents/common/api/agent_api.hpp"

namespace league::test::structured_economic_policy {

class Policy {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget,
             kag::Action& action);

private:
    kag::agent::AgentConfig config_{};
    int signature_last_step_ = -1;
    bool signature_active_ = false;
    int schedule_wheat_requested_ = 0;
};

}
