#pragma once
#include "../native_solver_api/exact.hpp"
#include "../native_solver_api_coarse/screen.hpp"
#include <functional>

namespace day_native::early {
struct Options {
    double seconds = 2, screen_seconds = 0.5, exact_seconds = 1;
    bool fix_source_profiles = false;
};
struct Backends {
    std::function<screen::Result(const day_solver::DayProblem&, const InternalHint&, const screen::SolveOptions&)> screen;
    std::function<exact::Result(const day_solver::DayProblem&, const exact::HintOptions&, const exact::SolveOptions&)> exact;
    std::function<double()> now;
};
struct Result {
    InternalHint ordered_hint;
    std::optional<screen::Result> coarse;
    std::optional<exact::Result> exact;
    double seconds = 0;
    bool accepted() const;
};
Backends native_backends();
// Only an internal proposal from OUR constructor is supplied alongside v3.
// Failure leaves the proposal and caller's fallback/cache unchanged.
Result complete(const day_solver::DayProblem&, const InternalHint&, const Options& = {},
                const Backends& = native_backends());
}
