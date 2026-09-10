#pragma once
#include "search.hpp"
#include "route_repair.hpp"
#include <iostream>

namespace placement {
struct Certificate {
    std::optional<Schedule> schedule;
    std::optional<DayProblem> problem;
    int workers = 41, queries = 0;
    double seconds = 0;
    std::string initial_witness = "none";
};

inline std::string nonlabor_contract(DayProblem problem) {
    problem.worker_count = 1;
    std::erase_if(problem.market_plan, [](const auto& event) { return event.market_op == kag::M_HIRE; });
    return day_solver::serialize_problem_json(problem);
}

inline Certificate warm_certificate(const Course& course, int day, const Layout& layout, const fs::path& folder) {
    Certificate result;
    if (!fs::exists(folder / "problem.json")) return result;
    auto problem = day_solver::load_problem_json(folder / "problem.json");
    if (nonlabor_contract(problem) != nonlabor_contract(course.problem(day, layout)))
        throw std::runtime_error("warm certificate belongs to a different physical contract");
    for (const auto& event : problem.market_plan) if (event.market_op == kag::M_HIRE) {
        const auto op = course.days[day].executable[event.hour].orders[event.order_index].op;
        if (op != kag::M_NONE && op != kag::M_HIRE) throw std::runtime_error("warm hire overwrites a fixed order");
    }
    day_scheduler::prepare_problem(problem);
    if (day == 29) labor::offline::require_terminal_work(problem);
    auto actions = labor::offline::read_actions((folder / "physical.actions.txt").string());
    const auto replay = day_solver::replay_schedule(problem, actions);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())
        throw std::runtime_error("invalid warm certificate");
    result.workers = problem.worker_count; result.problem = std::move(problem); result.schedule = actions; result.initial_witness = "warm";
    return result;
}

inline Certificate certify_day(const Course& course, int day, const Layout& layout,
                               double budget, double query_budget, const Certificate* initial = nullptr) {
    const auto began = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count(); };
    Certificate best;
    const auto base = course.problem(day, layout);
    const auto& source = course.days[day];
    const auto retained = day_solver::replay_schedule(base, source.physical);
    if (retained.candidate.replay.strict_valid && retained.requirements_satisfied &&
        retained.invariants_satisfied && retained.errors.empty()) {
        best.schedule = source.physical; best.problem = base; best.workers = source.problem.worker_count;
        best.initial_witness = "source";
    }
    for (int variant = 0; !best.schedule && variant < 4; ++variant)
        if (const auto repaired = route_repair(source, base, layout, variant)) {
            best.schedule = repaired; best.problem = base; best.workers = source.problem.worker_count;
            best.initial_witness = "rerouted";
        }
    if (initial && initial->schedule && initial->workers < best.workers) best = *initial;
    if (budget <= 0) { best.seconds = elapsed(); return best; }
    const auto forecast = fast_day_solver_estimator::estimate_day(base, source.menu);
    if (forecast.analytically_rejected) { best.seconds = elapsed(); return best; }
    std::array<bool, 41> attempted{};
    while (elapsed() < budget) {
        int next = 0;
        double utility = -1;
        for (int k = forecast.lower; k < best.workers; ++k) if (!attempted[k]) {
            // Until there is a witness, prefer the forecast neighborhood but
            // keep a high-workforce rescue available. UNKNOWN is never a proof.
            const double p = (forecast.forecasted & (uint64_t{1} << k)) ? forecast.probabilities[k] :
                1. / (1. + std::exp(-2. * (k - forecast.workers)));
            const double gain = best.schedule ? labor::hire_cost(best.workers) - labor::hire_cost(k) : 1.;
            const double rank = best.schedule ? p * gain : p / (1. + std::abs(k - forecast.workers));
            if (rank > utility) { utility = rank; next = k; }
        }
        if (!next) break;
        attempted[next] = true;
        auto problem = workforce(base, source.menu, next, day);
        day_scheduler::Options options;
        options.seconds = std::min(query_budget, std::max(.001, budget - elapsed()));
        options.fallback_workers = 1;
        const auto solved = day_scheduler::solve(problem, options);
        ++best.queries;
        if (solved.schedule) {
            best.schedule = solved.schedule; best.problem = std::move(problem); best.workers = next;
        }
    }
    best.seconds = elapsed();
    return best;
}

inline Layout read_layout(const fs::path& path) {
    std::ifstream input(path);
    Layout layout;
    for (auto& cell : layout) { int value; input >> value; if (!input || value < 0 || value >= 100) throw std::runtime_error("invalid layout"); cell = value; }
    std::string extra; if (input >> extra) throw std::runtime_error("extra layout content");
    return layout;
}
}
