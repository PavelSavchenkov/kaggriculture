#pragma once

#include <array>
#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::detail::reassignment {

struct Parameters {
    bool enabled = true;
    uint32_t day_mask = (uint32_t{1} << 11) | (uint32_t{1} << 17) |
        (uint32_t{1} << 19);
    uint8_t maximum_early = 0;
};

struct Stats {
    struct Event {
        int16_t target_step = 0;
        int16_t start_step = 0;
        int8_t target_x = 0;
        int8_t target_y = 0;
        int8_t unit = 0;
    };
    int observed_targets = 0;
    int covered_targets = 0;
    int matched_days = 0;
    int suppressed_hires = 0;
    int route_actions = 0;
    int route_digs = 0;
    int route_aborts = 0;
    int redundant_digs = 0;
    int shifted_services = 0;
    int abort_step = -1;
    int abort_unit = -1;
    int abort_expected_x = -1;
    int abort_expected_y = -1;
    int abort_actual_x = -1;
    int abort_actual_y = -1;
    std::array<Event, 12> events{};
    int event_count = 0;
};

class Overlay {
public:
    void reset(uint8_t player);
    void set_parameters(const Parameters& parameters) { parameters_ = parameters; }
    void modify(const kag::agent::AgentObservation& observation, kag::Action& action);
    const Stats& stats() const { return stats_; }

private:
    static constexpr int MAX_TARGETS = 12;
    static constexpr int MAX_DAY_ACTIONS = 24;

    struct Route {
        std::array<kag::UnitAction, MAX_DAY_ACTIONS> actions{};
        std::array<int8_t, MAX_DAY_ACTIONS> x{};
        std::array<int8_t, MAX_DAY_ACTIONS> y{};
        int16_t start = 0;
        uint8_t size = 0;
        bool active = false;
        int16_t target_step = 0;
        int8_t target_x = 0;
        int8_t target_y = 0;
    };

    uint8_t player_ = 0;
    int last_day_ = -1;
    Parameters parameters_{};
    Stats stats_{};
    std::array<Route, kag::MAX_UNITS> routes_{};
    bool matched_day_ = false;

    void begin_day(const kag::agent::AgentObservation& observation);
};

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::detail::reassignment
