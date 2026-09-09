#pragma once
#include "context_model.hpp"
#include "query_policy.hpp"

namespace labor {
inline double interpolate_hire_cost(double workers) {
    if (!std::isfinite(workers) || workers < 1 || workers > 40) throw std::runtime_error("invalid fractional workforce");
    const int k = int(workers);
    return k == 40 ? hire_cost(k) : hire_cost(k) + (workers - k) * (hire_cost(k + 1) - hire_cost(k));
}
struct CurveSummary {
    double peak = 0, mean_workers = 0, mean_cost = 0;
    std::array<int, 4> quantiles{};
    int absolute_half = 0;
};

inline CurveSummary summarize_curve(const std::array<double, 41>& probabilities, uint64_t allowed) {
    if (!allowed || (allowed & 1) || (allowed >> 41)) throw std::runtime_error("invalid curve workforce mask");
    CurveSummary result;
    for (int k = 1; k <= 40; ++k) if (allowed & (uint64_t{1} << k)) {
        const double p = probabilities[k];
        if (!std::isfinite(p) || p < 0 || p > 1) throw std::runtime_error("invalid curve probability");
        result.peak = std::max(result.peak, p);
    }
    if (result.peak <= 0) throw std::runtime_error("cannot normalize a zero-probability curve");
    const std::array<double, 4> cuts{.25, .5, .75, .9};
    double previous = 0;
    for (int k = 1; k <= 40; ++k) if (allowed & (uint64_t{1} << k)) {
        const double cumulative = std::max(previous, probabilities[k]);
        const double mass = (cumulative - previous) / result.peak;
        result.mean_workers += mass * k; result.mean_cost += mass * hire_cost(k);
        for (int q = 0; q < 4; ++q)
            if (!result.quantiles[q] && cumulative / result.peak >= cuts[q]) result.quantiles[q] = k;
        if (!result.absolute_half && cumulative >= std::min(.5, result.peak)) result.absolute_half = k;
        previous = cumulative;
    }
    return result;
}

inline bool ordinary_earliest_context(const day_solver::DayProblem& problem, const PlanningMenu& menu) {
    if (menu.hours != 24 || menu.size != 39 || menu.minimum_workers != 1) return false;
    const auto earliest = earliest_menu(problem);
    return menu.hires.hours == earliest.hours && menu.hires.slots == earliest.slots;
}

struct PlanningPrediction {
    bool analytically_rejected = false, uses_direct_cost = false, complete_curve = false;
    bool low_peak = false, weak_probe = false;
    int lower = 1, probe_workers = 0, query_evaluations = 0;
    double workers = std::numeric_limits<double>::quiet_NaN(), cost = std::numeric_limits<double>::quiet_NaN(), probe_probability = -1;
    CurveSummary curve;
    ContextFeatures features{};
    std::array<double, 41> probabilities{};
    uint64_t forecasted = 0;
};

// Pure prediction: no solver, schedule, source workforce or outcome access.
// Ordinary earliest menus retain the direct-cost point estimate. Other menus
// use the probability-curve mean workforce. Neither is a feasibility proof.
inline PlanningPrediction estimate_planning(const day_solver::DayProblem& problem,
        const PlanningMenu& menu, bool force_curve = false) {
    PlanningPrediction result;
    result.features = extract_context(problem, menu);
    const auto& f = result.features;
    result.lower = int(f[planning_lower]);
    result.analytically_rejected = result.lower > menu.size + 1 || f[deadline_missing_quantity] > 0 ||
        f[planning_supply_base + 1] > 0 || f[planning_release_base + 1] > 0 || f[planning_release_base + 2] > 0;
    if (result.analytically_rejected) return result;
    result.uses_direct_cost = ordinary_earliest_context(problem, menu);
    auto predict = [&](int k) {
        const uint64_t bit = uint64_t{1} << k;
        if (!(result.forecasted & bit)) {
            result.probabilities[k] = context_model::boost(context_query_features(f, k));
            result.forecasted |= bit; ++result.query_evaluations;
        }
        return result.probabilities[k];
    };
    if (result.uses_direct_cost) {
        Features base{}; std::copy_n(f.begin(), count, base.begin());
        result.workers = std::max(double(result.lower), cost_equivalent_workers(search_model::cost(base)));
        result.cost = interpolate_hire_cost(result.workers);
        result.probe_workers = std::clamp(int(std::ceil(result.workers)), result.lower, menu.size + 1);
        result.probe_probability = predict(result.probe_workers);
        result.weak_probe = result.probe_probability < .2;
        // One probability >= 0.5 proves that the model's peak is >= 0.5.
        // Otherwise evaluate the full curve before reporting its low-peak flag.
        if (!force_curve && result.probe_probability >= .5) return result;
    }
    uint64_t allowed = 0;
    for (int k = result.lower; k <= menu.size + 1; ++k) {
        allowed |= uint64_t{1} << k; predict(k);
    }
    result.curve = summarize_curve(result.probabilities, allowed);
    result.complete_curve = true; result.low_peak = result.curve.peak < .5;
    if (!result.uses_direct_cost) {
        result.workers = std::clamp(result.curve.mean_workers, double(result.lower), double(menu.size + 1));
        result.cost = interpolate_hire_cost(result.workers);
    }
    return result;
}
}
