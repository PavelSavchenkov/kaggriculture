#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace league::public_unchanged::c68 {

struct WeedTransaction {
    bool active = false;
    int start = -1;
    kag::UnitAction intended{};
};

struct RaceState {
    int last_step = -1;
    bool has_inventory = false;
    std::array<int, kag::N_PRODUCTS> inventory{};
    std::array<int, kag::N_PRODUCTS> own_sells{};
    std::array<uint8_t, kag::MAX_SHOP_INSTANCES> shops{};
    int n_shops = 0;
    std::array<double, 7> scores{};
    int events = 0;
    int horizon = 4;
};

struct SeatState {
    int weed_last_step = -1;
    std::array<WeedTransaction, kag::MAX_UNITS> weeds{};
    int shift_last_step = -1;
    std::array<std::array<int, 4>, 720> debts{};
    RaceState race{};
};

class Policy {
public:
    explicit Policy(bool preempt_enabled = true)
        : preempt_enabled_(preempt_enabled) {}
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget,
             kag::Action& action);

private:
    bool preempt_enabled_ = true;
    std::array<SeatState, 2> seats_{};
};

}
