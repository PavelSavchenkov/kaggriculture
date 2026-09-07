#include "day_solver/scheduler.hpp"
#include "components/native_quick_portfolio_v30/quick.hpp"

namespace day_scheduler {
Result solve(const day_solver::DayProblem& problem, const Options& options) {
    day_native::quick_v30::Options settings;
    settings.iterations = 64;
    settings.variants = 8;
    settings.rounds = 2;
    settings.fallback = true;
    settings.reference.seconds = options.seconds;
    settings.reference.workers = options.fallback_workers;
    auto result = day_native::quick_v30::solve(problem, settings);
    if (result.accepted()) return {std::move(result.winner->schedule), result.seconds};
    return {{}, result.seconds};
}
}
