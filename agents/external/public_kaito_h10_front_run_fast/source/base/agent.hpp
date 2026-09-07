#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace league::public_unchanged::kaito {

struct WeedTransaction {
    bool active = false;
    int start = -1;
    kag::UnitAction intended{};
};

struct SeatState {
    int last_step = -1;
    std::array<WeedTransaction, kag::MAX_UNITS> weeds{};
};

class Policy {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget,
             kag::Action& action);

private:
    std::array<SeatState, 2> seats_{};
};

}
