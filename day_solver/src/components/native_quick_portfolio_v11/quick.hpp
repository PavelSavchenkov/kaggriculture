#pragma once
#include "../native_public_portfolio/portfolio.hpp"

namespace day_native::quick_v11 {
struct Options {
    int iterations = 128, variants = 2;
    double route_seconds = 0.15, completion_seconds = 0.6;
    bool fallback = false, compact = true;
    int exact_tuning = 2, screen_tuning = 2, rounds = 1, repair_steps = -1;
    portfolio::Options reference;
};
portfolio::Result solve(const day_solver::DayProblem&, const Options& = {});
}
