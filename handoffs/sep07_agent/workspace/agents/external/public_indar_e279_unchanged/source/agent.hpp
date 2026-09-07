#pragma once

#include <array>

#include "agents/common/api/agent_api.hpp"

namespace league::public_unchanged::indar {

struct WeedTransaction {
    bool active = false;
    int start = -1;
    kag::UnitAction intended{};
};

struct SeatState {
    int selector_last_step = -1;
    int selector_expert = -1;
    std::array<uint8_t, kag::MAX_SHOP_INSTANCES> selector_shops{};
    int selector_n_shops = 0;
    int weed_last_step = -1;
    std::array<WeedTransaction, kag::MAX_UNITS> weeds{};
    int r5_last_step = -1;
    bool r5_target = false;
    int md_last_step = -1;
    bool md_target = false;
    int room_last_step = -1;
    int room_day = -1;
    int room_actor = -1;
    int room_target_x = 0;
    int room_target_y = 0;
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
