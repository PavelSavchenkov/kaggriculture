#pragma once

#include <array>
#include <cstdint>

#include "agents/common/api/observation.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza {

inline constexpr int SOURCE_ANIMALS = 23;

struct Genome {
    uint8_t cows = 2;             // 0..23
    uint8_t sheep = 4;            // 0..23-cows; the balance is geese
    uint8_t animal_order = 0;     // 0 front-load cows, 1 balanced, 2 late cows
    uint8_t tomato_mode = 0;      // 0 unchanged, 1 strawberry -> tomato
    uint8_t market_mode = 0;      // 0 preserve, 1 periodic, 2 capacity-only
    uint8_t sale_period = 4;      // 1..24 turns
    uint16_t milk_floor = 160;    // 1..1000
    uint16_t tomato_floor = 60;   // 1..1000
    uint8_t shed_pressure = 82;   // 1..100; force target sales above it
    uint8_t late_cow_additions = 0; // 0 base, 1 group13, 2 also group10
    int8_t cow_group_override = -1; // -1 none; 0..14 force one group to cow
    bool enabled = true;
};

bool valid(const Genome& genome);

class RemapPolicy {
public:
    explicit RemapPolicy(const Genome& genome);

    void reset();
    void modify(const kag::agent::AgentObservation& observation,
                kag::Action& action);

private:
    Genome genome_;
    std::array<uint8_t, SOURCE_ANIMALS> desired_{};
    int buy_cursor_ = 0;
    std::array<std::array<uint8_t, SOURCE_ANIMALS>, 3> purchase_queue_{};
    std::array<uint8_t, 3> purchase_head_{};
    std::array<uint8_t, 3> purchase_tail_{};
    std::array<std::array<std::array<uint8_t, SOURCE_ANIMALS>, 3>,
               kag::MAX_UNITS> unit_queue_{};
    std::array<std::array<uint8_t, 3>, kag::MAX_UNITS> unit_head_{};
    std::array<std::array<uint8_t, 3>, kag::MAX_UNITS> unit_tail_{};

    void build_desired();
    void remap_orders(const kag::agent::AgentObservation& observation,
                      kag::Action& action);
    void remap_units(const kag::agent::AgentObservation& observation,
                     kag::Action& action);
    void schedule_sales(const kag::agent::AgentObservation& observation,
                        kag::Action& action);
};

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza
