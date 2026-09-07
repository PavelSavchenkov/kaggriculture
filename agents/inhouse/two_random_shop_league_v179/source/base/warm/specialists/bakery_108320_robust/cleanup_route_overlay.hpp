#pragma once

#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::bakery_108320_robust::detail::cleanup {

struct Stats {
    int started = 0;
    int completed = 0;
    int aborts = 0;
    int suppressed_hires = 0;
    int overflow_sales = 0;
};

class Overlay {
public:
    void reset(uint8_t player);
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_route265_enabled(bool enabled) { route265_enabled_ = enabled; }
    void set_route121_enabled(bool enabled) { route121_enabled_ = enabled; }
    void set_route241_enabled(bool enabled) { route241_enabled_ = enabled; }
    void set_overflow599_enabled(bool enabled) { overflow599_enabled_ = enabled; }
    void modify(const kag::agent::AgentObservation& observation,
                kag::Action& action);
    const Stats& stats() const { return stats_; }

private:
    uint8_t player_ = 0;
    bool enabled_ = false;
    bool route265_enabled_ = false;
    bool route121_enabled_ = false;
    bool route241_enabled_ = false;
    bool overflow599_enabled_ = false;
    bool route265_active_ = false;
    bool route121_active_ = false;
    bool route241_active_ = false;
    bool route457_active_ = false;
    Stats stats_{};

    void apply_route265(const kag::agent::AgentObservation& observation,
                        kag::Action& action);
    void apply_route121(const kag::agent::AgentObservation& observation,
                        kag::Action& action);
    void apply_route241(const kag::agent::AgentObservation& observation,
                        kag::Action& action);
    void apply_route457(const kag::agent::AgentObservation& observation,
                        kag::Action& action);
    void apply_overflow599(const kag::agent::AgentObservation& observation,
                           kag::Action& action);
};

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::bakery_108320_robust::detail::cleanup
