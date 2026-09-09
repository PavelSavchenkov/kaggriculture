#pragma once
#include "planning_estimator.hpp"

namespace fast_day_solver_estimator {
using Menu = labor::PlanningMenu;
using Estimate = labor::PlanningPrediction;

inline Menu earliest_hiring_menu(const day_solver::DayProblem& problem, int active_hours = 24) {
    const auto hires = labor::earliest_menu(problem, active_hours);
    std::array<std::pair<int, int>, 39> optional;
    for (int i = 0; i < 39; ++i) optional[i] = {hires.hours[i], hires.slots[i]};
    return labor::fixed_planning_menu(problem, active_hours, {}, optional);
}

// Uses no solver calls or schedule/workforce answers. Check analytically_rejected
// before reading cost/workers. A point estimate is not a feasible certificate.
inline Estimate estimate_day(const day_solver::DayProblem& problem, const Menu& menu) {
    return labor::estimate_planning(problem, menu);
}

inline Estimate estimate_day(const day_solver::DayProblem& problem) {
    return estimate_day(problem, earliest_hiring_menu(problem));
}

inline std::optional<double> marginal_cost(const Estimate& baseline, const Estimate& candidate) {
    if (baseline.analytically_rejected || candidate.analytically_rejected) return std::nullopt;
    return candidate.cost - baseline.cost;
}
}
