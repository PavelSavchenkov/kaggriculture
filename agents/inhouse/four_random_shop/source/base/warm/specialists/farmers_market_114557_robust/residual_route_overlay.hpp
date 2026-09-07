#pragma once

#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::four_random_shop::base::warm::farmers_market_114557_robust::detail::residual_route {

struct Parameters {
    uint32_t suppress_day_mask = uint32_t{1} << 17;
    uint16_t target_mask = 74;
    bool require_single_target = true;
    bool repair_target1 = true;
    bool repair_target6 = true;
    bool suppress_hires = true;
};

struct Stats {
    int suppressed_hires = 0;
    int target1_repairs = 0;
    int target6_repairs = 0;
    int target6_companion_fallbacks = 0;
    int completed_routes = 0;
    int route_failures = 0;
};

class Overlay {
public:
    void reset(uint8_t player);
    void set_parameters(const Parameters& parameters) { parameters_ = parameters; }
    void modify(const kag::agent::AgentObservation& observation,
                kag::Action& action);
    const Stats& stats() const { return stats_; }

private:
    uint8_t player_ = 0;
    int suppressed_target_ = -1;
    bool target1_check_ = false;
    bool target6_route_ = false;
    Parameters parameters_{};
    Stats stats_{};

    void verify_route(const kag::agent::AgentObservation& observation);
    void apply_target6_route(const kag::agent::AgentObservation& observation,
                             kag::Action& action);
};

}  // namespace kag::agents::four_random_shop::base::warm::farmers_market_114557_robust::detail::residual_route
