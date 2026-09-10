#pragma once
#include "certify.hpp"

namespace placement {
inline Certificate solve_contract(const Day& day, int d, double budget, double query_budget, const Certificate* incumbent = nullptr) {
    Certificate best;
    if (incumbent && incumbent->schedule) {
        if (nonlabor_contract(*incumbent->problem) != nonlabor_contract(day.problem)) throw std::runtime_error("stale contract incumbent");
        const auto replay = day_solver::replay_schedule(*incumbent->problem, *incumbent->schedule);
        if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())
            throw std::runtime_error("invalid contract incumbent");
        best = *incumbent;
    }
    const auto began = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count(); };
    if (budget <= 0) return best;
    const auto forecast = fast_day_solver_estimator::estimate_day(day.problem, day.menu);
    if (forecast.analytically_rejected) return best;
    std::array<bool, 41> attempted{};
    while (elapsed() < budget) {
        int next = 0; double utility = -1;
        for (int k = forecast.lower; k < best.workers; ++k) if (!attempted[k]) {
            const double p = (forecast.forecasted & (uint64_t{1} << k)) ? forecast.probabilities[k] : 1. / (1. + std::exp(-2. * (k - forecast.workers)));
            const double rank = best.schedule ? p * (labor::hire_cost(best.workers) - labor::hire_cost(k)) : p / (1. + std::abs(k - forecast.workers));
            if (rank > utility) { utility = rank; next = k; }
        }
        if (!next) break;
        attempted[next] = true;
        auto problem = workforce(day.problem, day.menu, next, d);
        day_scheduler::Options options; options.seconds = std::min(query_budget, std::max(.001, budget - elapsed())); options.fallback_workers = 1;
        const auto solved = day_scheduler::solve(problem, options); ++best.queries;
        if (solved.schedule) { best.schedule = solved.schedule; best.problem = std::move(problem); best.workers = next; }
    }
    best.seconds = elapsed();
    return best;
}
}
