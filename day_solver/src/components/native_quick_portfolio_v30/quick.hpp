#pragma once
#include "../native_public_portfolio/portfolio.hpp"

namespace day_native::quick_v30 {
struct Options {
    int iterations = 128, variants = 2;
    double route_seconds = 0.15, completion_seconds = 1.5, screen_seconds = 0.9, retry_seconds = 4;
    double geometry_seconds = 1;
    int geometry_after = 2;
    bool fallback = false, compact = true, modern_fallback = true, materialize = true;
    int exact_tuning = 2, screen_tuning = 2, rounds = 1;
    portfolio::Options reference;
};
portfolio::Result solve(const day_solver::DayProblem&, const Options& = {});
}
