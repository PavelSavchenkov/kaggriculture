#pragma once

#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::smoothie_142388_robust::detail::residual {

struct Parameters {
    uint32_t suppress_cleanup_hire_day_mask = 0;
    uint16_t day17_target_mask = 0;
    bool require_single_day17_target = true;
};

struct Stats {
    int suppressed_cleanup_hires = 0;
    int rejected_multiple_targets = 0;
    int rejected_target_mask = 0;
    uint64_t suppression_weed_mask[2] = {0, 0};
    int suppression_weed_count = 0;
};

class Overlay {
public:
    void reset(uint8_t player);
    void set_parameters(const Parameters& parameters) { parameters_ = parameters; }
    void modify(const kag::agent::AgentObservation& observation, kag::Action& action);
    const Stats& stats() const { return stats_; }

private:
    uint8_t player_ = 0;
    Parameters parameters_{};
    Stats stats_{};
};

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::smoothie_142388_robust::detail::residual
