#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace league::public_unchanged::boatlee {

struct WeedTransaction {
    bool active = false;
    int start = -1;
    kag::UnitAction intended{};
};

struct TapeSeatState {
    int weed_last_step = -1;
    std::array<WeedTransaction, kag::MAX_UNITS> weeds{};
    int front_last_step = -1;
    int due_step = -1;
    std::array<int, kag::N_PRODUCTS> due{};
};

struct RaceState {
    int last_step = -1;
    bool has_inventory = false;
    std::array<int, kag::N_PRODUCTS> inventory{};
    std::array<int, kag::N_PRODUCTS> prices{};
    std::array<int, kag::N_PRODUCTS> own_sells{};
    std::array<uint8_t, kag::MAX_SHOP_INSTANCES> shops{};
    int n_shops = 0;
    std::array<std::array<double, 7>, 4> scores{};
    std::array<double, 4> evidence{};
    std::array<int, 4> horizon{};
    std::array<double, 7> policy_scores{};
    double policy_evidence = 0;
    int policy_horizon = 1;
};

struct MoonSeatState {
    int layout = -1;
    int weed_last_step = -1;
    std::array<WeedTransaction, kag::MAX_UNITS> weeds{};
    int shift_last_step = -1;
    std::array<std::array<int, 4>, 720> debts{};
    RaceState race{};
    int r5_last_step = -1;
    bool r5_target = false;
    int md_last_step = -1;
    bool md_target = false;
    int room_day = -1;
    int room_last_step = -1;
    int room_actor = -1;
    int room_target_x = 0;
    int room_target_y = 0;
    int tomato_last_step = -1;
    bool tomato_active = false;
    int tomato_scheduled_plants = 0;
    int tomato_seed_debt = 0;
    int egg_last_step = -1;
    bool egg_active = false;
};

struct SeatState {
    int route = 0;
    bool market_overlay = false;
    bool has_previous_opponent_money = false;
    double previous_opponent_money = 0;
    MoonSeatState moon{};
    TapeSeatState mutoy{};
    TapeSeatState munib_base{};
    TapeSeatState munib_front{};
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
