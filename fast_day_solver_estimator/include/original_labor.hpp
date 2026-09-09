#pragma once
#include <day_solver/scheduler.hpp>
#include <algorithm>
#include <cmath>

namespace labor::original {
struct Aggregate { int operations = 0, farthest = 0; double distance_work = 0; };
struct Estimate { int work_turns = 0, workers = 1, cost = 0; };

// Exact labor arithmetic from estimate_plan. Inputs are its daily aggregates.
inline Estimate estimate(const Aggregate& a, int max_hands = 20) {
    Estimate result;
    result.work_turns = a.operations + int(std::ceil(a.distance_work)) + 2 * a.farthest;
    const int needed = std::max(0, (result.work_turns + 21) / 22 - 1);
    const int hands = std::min(max_hands, needed);
    result.workers = hands + 1;
    for (int i = 0; i < hands; ++i) result.cost += kag::fib(i);
    return result;
}

// Adapter to explicit DayProblem work. It bypasses the original composition's
// biological forecast, preserving its labor formula and default assumptions.
inline Aggregate aggregate(const day_solver::DayProblem& p) {
    Aggregate a;
    for (const auto& work : p.tile_work) {
        const auto& tile = p.start.managed_tiles[work.tile];
        const int d = std::min(std::abs(tile.x - 4), std::abs(tile.x - 5))
                    + std::min(std::abs(tile.y - 4), std::abs(tile.y - 5));
        int output = 0, wheat = 0, fertilizer = 0;
        for (const auto& action : work.actions) {
            output += action.output_quantity;
            wheat += action.op == kag::OP_FEED;
            fertilizer += action.op == kag::OP_FERTILIZE;
        }
        a.operations += work.actions.size();
        if (!work.actions.empty()) a.farthest = std::max(a.farthest, d);
        a.distance_work += work.actions.size();
        a.distance_work += (output + wheat + fertilizer) * double(2 * d + 1) / 12;
    }
    return a;
}

// Exact general_model.hpp additive labor term. Use differences in operation
// counts for marginal comparisons; do not pretend it was a workforce model.
inline double marginal_flat(int candidate_operations, int baseline_operations, double coefficient = 10) {
    return coefficient * (candidate_operations - baseline_operations);
}
}
