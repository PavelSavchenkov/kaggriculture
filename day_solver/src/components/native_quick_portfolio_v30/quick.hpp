#pragma once
#include "../native_public_portfolio/portfolio.hpp"
#include <functional>

namespace day_native::quick_v30 {
struct Options {
    int iterations = 128, variants = 2;
    double route_seconds = 0.15, completion_seconds = 1.5, screen_seconds = 0.9, retry_seconds = 4;
    double geometry_seconds = 1;
    int geometry_after = 2;
    bool fallback = false, compact = true, modern_fallback = true, materialize = true;
    int exact_tuning = 2, screen_tuning = 2, rounds = 1;
    portfolio::Options reference;
    // An outer physical contract can reject a relaxed candidate without
    // stopping the remaining portfolio variants.
    std::function<bool(const std::array<kag::Action, day_solver::HOURS>&)> accept_schedule;
    // Use the actual storage contract for job completion when the other
    // constructors operate on a lossless routing relaxation.
    const day_solver::DayProblem* job_problem = nullptr;
};
portfolio::Result solve(const day_solver::DayProblem&, const Options& = {});
}
