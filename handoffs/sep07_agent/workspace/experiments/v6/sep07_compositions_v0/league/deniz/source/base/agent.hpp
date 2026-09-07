#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace four_shop_foundry::public_unchanged::deniz_v111 {

class Agent {
public:
    explicit Agent(bool front_run = true) : front_run_(front_run) {}
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    bool front_run_ = true;
    uint8_t player_ = 0;
    int last_step_ = -1;
    int due_step_ = -1;
    std::array<int, kag::N_ITEMS> due_{};
    std::array<int16_t, kag::MAX_UNITS> repair_start_{};
    std::array<kag::UnitAction, kag::MAX_UNITS> repair_intended_{};
};

}
