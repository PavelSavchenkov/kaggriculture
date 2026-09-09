#pragma once
#include <day_solver/scheduler.hpp>
#include <stdexcept>
#include <utility>

namespace labor::offline {
// The v3 JSON describes a 24-phase day. These additional in-memory bounds tell
// the unchanged solver that all required field work must occur by phase 22.
// The virtual final phase remains only for public day-end totals.
inline void require_terminal_work(day_solver::DayProblem& p) {
    using namespace day_solver;
    using namespace kag;
    for (const auto& event : p.market_plan)
        if (event.hour >= 23) throw std::runtime_error("terminal purchase in virtual phase");
    if (p.shed_availability[23] != p.shed_availability[22]) throw std::runtime_error("terminal withdrawal in virtual phase");
    const std::pair<int, OutcomeMetric> metrics[] = {
        {OP_PLANT, OutcomeMetric::PLANTED}, {OP_WATER, OutcomeMetric::CROP_WATERED},
        {OP_HARVEST, OutcomeMetric::HARVESTED}, {OP_FERTILIZE, OutcomeMetric::CROP_FERTILIZED},
        {OP_DIG, OutcomeMetric::DUG}, {OP_BUILD_PASTURE, OutcomeMetric::PASTURE_BUILT},
        {OP_BUILD_COOP, OutcomeMetric::COOP_BUILT}, {OP_PLACE, OutcomeMetric::ANIMAL_PLACED},
        {OP_FEED, OutcomeMetric::FED}, {OP_CARE, OutcomeMetric::CARED},
        {OP_COLLECT_FERTILIZER, OutcomeMetric::FERTILIZER_COLLECTED}};
    for (const auto& [operation, metric] : metrics) {
        int64_t count = 0;
        for (const auto& work : p.tile_work) for (const auto& action : work.actions)
            if (action.op == operation) count += operation == OP_HARVEST ? action.output_quantity : 1;
        if (count) p.required_outcomes.push_back({{metric, NO_SUBJECT, NO_TILE, 22}, count, std::numeric_limits<int64_t>::max()});
    }
}
}
