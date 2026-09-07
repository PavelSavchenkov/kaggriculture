#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace league::public_unchanged::roman_hamburger {

struct PendingPasture {
    bool active = false;
    bool farmer = false;
    int actor = -1;
    int x = -1;
    int y = -1;
    int expected_step = -1;
};

struct PendingPlant {
    bool active = false;
    int actor = -1;
    int crop = kag::N_ITEMS;
    int x = -1;
    int y = -1;
    int expected_step = -1;
};

struct PendingWater {
    bool active = false;
    int planter = -1;
    int x = -1;
    int y = -1;
    int expected_step = -1;
};

struct SeatState {
    PendingPasture pasture{};
    int farmer_shift_end = -1;
    PendingPlant plant{};
    PendingWater water{};
    int water_shift_actor = -1;
    int water_shift_end = -1;
    bool soil_repair_activated = false;
    bool cashflow_active = false;
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
