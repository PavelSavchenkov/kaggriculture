#pragma once

#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::four_random_shop::base::warm::farmers_market_114557_robust::detail::target8 {

enum class Mode : uint8_t {
    off,
    suppress,
    route,
    route_endpoint_guard,
    route_companion_guard,
    route_closed,
    route_safe,
    route_strict,
    route_single_target,
};

struct Stats {
    int decisions = 0;
    int suppressed_hires = 0;
    int endpoint_fallbacks = 0;
    int companion_fallbacks = 0;
    int companion_routes = 0;
    int multiple_fallbacks = 0;
    int conflict_fallbacks = 0;
    int route_started = 0;
    int route_completed = 0;
    int route_aborts = 0;
};

struct Decision {
    bool triggered = false;
    int step = -1;
    int target_count = 0;
    int all_weed_count = 0;
    int highest_target = -1;
    int cash = 0;
    uint64_t weed_lo = 0;
    uint64_t weed_hi = 0;
};

class Overlay {
public:
    void reset(uint8_t player);
    void set_mode(Mode mode) { mode_ = mode; }
    void modify(const kag::agent::AgentObservation& observation,
                kag::Action& action);
    const Stats& stats() const { return stats_; }
    const Decision& decision() const { return decision_; }

private:
    uint8_t player_ = 0;
    Mode mode_ = Mode::off;
    bool route_active_ = false;
    bool route_companion_ = false;
    bool route_completion_pending_ = false;
    Stats stats_{};
    Decision decision_{};

    void apply_route(const kag::agent::AgentObservation& observation,
                     kag::Action& action);
};

}  // namespace kag::agents::four_random_shop::base::warm::farmers_market_114557_robust::detail::target8
