#pragma once

#include <array>
#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::yarn_125268_robust::detail::donor {

struct DonorParameters {
    bool enabled = true;
    uint32_t day_mask = (uint32_t{1} << 30) - 1;
    uint32_t kind_mask = (uint32_t{1} << (kag::N_CROPS + 2)) - 1;
    uint8_t minimum_slack = 0;
    bool suppress_redundant_cleanup_hire = false;
    uint8_t maximum_lead_days = 0;
};

struct DonorStats {
    struct Event {
        int16_t step = 0;
        int16_t deadline = 0;
        int16_t rejoin = 0;
        int8_t unit = 0;
        int8_t x = 0;
        int8_t y = 0;
    };
    int plans = 0;
    int digs = 0;
    int targets = 0;
    int available_windows = 0;
    int route_steps = 0;
    int suppressed_hires = 0;
    std::array<Event, 8> events{};
    int event_count = 0;
};

class DonorOverlay {
public:
    void reset(uint8_t player);
    void set_parameters(const DonorParameters& parameters) { parameters_ = parameters; }
    void modify(const kag::agent::AgentObservation& observation, kag::Action& action);
    const DonorStats& stats() const { return stats_; }

private:
    static constexpr int MAX_TARGETS = 32;
    static constexpr int MAX_ROUTE = 24;

    struct Target {
        int16_t deadline = 0;
        int8_t x = 0;
        int8_t y = 0;
        uint8_t kind = 0;
        bool active = false;
        bool reserved = false;
    };

    struct Detour {
        std::array<kag::UnitAction, MAX_ROUTE> actions{};
        uint8_t size = 0;
        uint8_t cursor = 0;
        int8_t target = -1;

        bool active() const { return cursor < size; }
        void clear() { size = cursor = 0; target = -1; }
    };

    uint8_t player_ = 0;
    int last_day_ = -1;
    DonorParameters parameters_{};
    DonorStats stats_{};
    std::array<Target, MAX_TARGETS> targets_{};
    int target_count_ = 0;
    std::array<Detour, kag::MAX_UNITS> detours_{};

    void begin_day(const kag::agent::AgentObservation& observation);
    bool plan(const kag::agent::AgentObservation& observation,
              const kag::Action& base, int unit);
};

}  // namespace kag::agents::yarn_125268_robust::detail::donor
