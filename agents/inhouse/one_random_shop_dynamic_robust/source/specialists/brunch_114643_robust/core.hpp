#pragma once

#include <array>
#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::brunch_114643_robust::detail {

struct RepairParameters {
    bool queue_water_after_repaired_plant = true;
    bool repair_animal_place = true;
    bool clamp_market_orders = true;
    bool cleanup_worker = true;
    uint32_t cleanup_day_mask = 665632;
    uint32_t cleanup_kind_mask = 113;
    uint8_t cleanup_late_hour = 24;
    uint32_t cleanup_multi_day_mask = 1024;
    uint8_t cleanup_multi_min_targets = 2;
    uint32_t cleanup_recovery_day_mask = 0;
    uint32_t cleanup_value_day_mask = 526336;
    uint16_t cleanup_day17_target_mask = 25947;
    uint8_t cleanup_lookahead_days = 0;
    uint32_t cleanup_lookahead_kind_mask =
        (uint32_t{1} << (kag::N_CROPS + 2)) - 1;
    bool catch_up_sales = false;
    bool liquidity_sales = false;
    int8_t liquidity_preferred_item = -1;
    bool liquidity_opportunity_selector = false;
    bool underfilled_sale_substitution = false;
};

struct RepairStats {
    struct FundingEvent {
        int16_t step = -1;
        uint8_t purchase_op = 0;
        uint8_t purchase_item = 0;
        int16_t purchase_n = 0;
        int8_t sale_item = -1;
        int16_t sale_n = 0;
        double cash_before = 0;
        double required = 0;
        double proceeds = 0;
        double estimated_revenue_delta = 0;
    };
    int weed_digs = 0;
    int repaired_plants = 0;
    int repaired_builds = 0;
    int repaired_places = 0;
    int queued_actions = 0;
    int stale_actions = 0;
    int day_boundary_drops = 0;
    int maximum_queue = 0;
    int cleanup_hires = 0;
    int cleanup_digs = 0;
    int catch_up_sold = 0;
    int liquidity_sold = 0;
    int substituted_sales = 0;
    std::array<int, 30> liquidity_sold_by_day{};
    std::array<int, kag::N_PRODUCTS> liquidity_sold_by_item{};
    std::array<FundingEvent, 4> funding_events{};
    uint8_t funding_event_count = 0;
};

class CoreAgent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(
        const kag::agent::AgentObservation& observation,
        const kag::agent::DecisionBudget& budget,
        kag::Action& action
    );
    void set_parameters(const RepairParameters& parameters) { parameters_ = parameters; }
    const RepairStats& stats() const { return stats_; }

private:
    static constexpr int QUEUE_CAPACITY = 64;
    static constexpr int EXACT_CLEANUP_TARGETS = 12;
    static constexpr int CLEANUP_DP_STATES = 1 << EXACT_CLEANUP_TARGETS;

    struct Queue {
        std::array<kag::UnitAction, QUEUE_CAPACITY> actions{};
        uint8_t size = 0;

        void clear() { size = 0; }
        void push_back(kag::UnitAction action);
        void push_front(kag::UnitAction action);
        kag::UnitAction pop_front();
    };

    struct CleanupTarget {
        int16_t deadline = 0;
        int8_t x = 0;
        int8_t y = 0;
        uint8_t weight = 1;
        bool active = false;
        bool hire_eligible = false;
    };

    uint8_t player_ = 0;
    int last_day_ = -1;
    RepairParameters parameters_{};
    RepairStats stats_{};
    std::array<Queue, kag::MAX_UNITS> queues_{};
    std::array<CleanupTarget, kag::BOARD * kag::BOARD> cleanup_targets_{};
    int cleanup_target_count_ = 0;
    int cleanup_current_target_ = -1;
    bool cleanup_hire_requested_ = false;
    std::array<int32_t, kag::N_PRODUCTS> actual_sold_{};
    std::array<int16_t, CLEANUP_DP_STATES * EXACT_CLEANUP_TARGETS> cleanup_finish_{};
    std::array<int8_t, CLEANUP_DP_STATES * EXACT_CLEANUP_TARGETS> cleanup_first_{};
};

}  // namespace kag::agents::brunch_114643_robust::detail
