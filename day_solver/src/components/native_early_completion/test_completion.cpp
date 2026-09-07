#include "completion.hpp"
#include <cmath>
#include <iostream>
#include <tuple>

using namespace day_native;
namespace sat = operations_research::sat;
static void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static auto assignments(const InternalHint& hint) {
    std::vector<std::tuple<int, int, int, int>> result;
    for (const auto& row : hint.assignments) result.emplace_back(row.task, row.worker, required(row.hour, "hour"), row.type.value_or(0));
    return result;
}
struct Scenario {
    sat::CpSolverStatus coarse, exact;
    bool replay, profiles;
    double total, coarse_elapsed;
    bool expect_exact, accepted;
};
int main() {
    try {
        const std::vector<Scenario> scenarios{
            {sat::UNKNOWN, sat::OPTIMAL, true, false, 2, 0.4, false, false},
            {sat::INFEASIBLE, sat::OPTIMAL, true, false, 2, 0.4, false, false},
            {sat::OPTIMAL, sat::UNKNOWN, false, false, 2, 0.4, true, false},
            {sat::OPTIMAL, sat::INFEASIBLE, false, false, 2, 0.4, true, false},
            {sat::OPTIMAL, sat::OPTIMAL, false, false, 2, 0.4, true, false},
            {sat::OPTIMAL, sat::OPTIMAL, true, false, 2, 0.4, true, true},
            {sat::FEASIBLE, sat::FEASIBLE, true, true, 2, 0.4, true, true},
            {sat::OPTIMAL, sat::OPTIMAL, true, false, 0.7, 0.4, true, true},
            {sat::OPTIMAL, sat::OPTIMAL, true, false, 0.3, 0.4, false, false},
            {sat::OPTIMAL, sat::OPTIMAL, true, false, 0, 0, false, false},
        };
        int controls = 0;
        for (const auto& scenario : scenarios) {
            day_solver::DayProblem problem;
            problem.worker_count = 4;
            InternalHint source{{{1, 3, 5, 1, {}}, {0, 3, 2, 1, {}}, {2, 1, 10, 0, {}}},
                                std::vector<std::vector<int>>{{1}, {3}}, false};
            const auto saved = assignments(source);
            const auto saved_groups = source.type_workers;
            double clock = 100;
            int coarse_calls = 0, exact_calls = 0;
            early::Options options;
            options.seconds = scenario.total; options.fix_source_profiles = scenario.profiles;
            const std::vector<std::tuple<int, int, int, int>> expected{{2, 1, 0, 0}, {0, 3, 0, 1}, {1, 3, 1, 1}};
            InternalHint screened;
            early::Backends backends{
                [&](const auto& supplied, const InternalHint& hint, const screen::SolveOptions& settings) {
                    ++coarse_calls;
                    check(supplied.worker_count == 4, "input changed");
                    check(assignments(hint) == expected && hint.type_workers == saved_groups, "source order/labels changed");
                    check(settings.workers == 1 && settings.seed == 0, "coarse search parameters changed");
                    check(std::abs(settings.seconds - std::min(0.5, scenario.total)) < 1e-8, "coarse budget not clipped");
                    const auto& model = settings.model;
                    check(model.dynamic && model.fixed_order && model.fixed_profile == scenario.profiles, "coarse options changed");
                    check(!model.ignore_availability && !model.ignore_pickups && !model.ignore_precedence && !model.soft_precedence &&
                          !model.soft_availability && !model.fixed_availability && model.free_order.empty(), "coarse relaxation introduced");
                    clock += scenario.coarse_elapsed;
                    screen::Result result;
                    result.solved = true; result.status = scenario.coarse;
                    if (scenario.coarse == sat::OPTIMAL || scenario.coarse == sat::FEASIBLE) {
                        screened = hint;
                        for (auto& row : screened.assignments) row.hour = *row.hour + 7;
                        result.hint = screened;
                    }
                    return result;
                },
                [&](const auto&, const exact::HintOptions& hint, const exact::SolveOptions& settings) {
                    ++exact_calls;
                    check(hint.documents.size() == 1 && hint.documents.contains(HintKind::fixed_partial), "wrong exact restriction");
                    check(assignments(hint.documents.at(HintKind::fixed_partial)) == assignments(screened), "coarse assignment changed");
                    check(hint.flags.empty() && hint.free_tasks.empty() && hint.free_routes.empty(), "extra exact restriction/release");
                    check(settings.workers == 1 && !settings.earliest, "wrong exact search parameters");
                    check(std::abs(settings.seconds - std::min(1.0, scenario.total - scenario.coarse_elapsed)) < 1e-8, "exact budget not clipped");
                    clock += 0.2;
                    exact::Result result;
                    result.solved = true; result.status = scenario.exact;
                    if (scenario.exact == sat::OPTIMAL || scenario.exact == sat::FEASIBLE) {
                        result.schedule.emplace();  // Scripted control, not a replay-validity claim.
                        result.replay = exact::ReplayCheck{scenario.replay, scenario.replay, scenario.replay, scenario.replay, {}};
                    }
                    return result;
                },
                [&] { return clock; }
            };
            const auto result = early::complete(problem, source, options, backends);
            check(coarse_calls == int(scenario.total > 0) && exact_calls == int(scenario.expect_exact), "wrong call sequence");
            check(result.accepted() == scenario.accepted, "unverified schedule accepted");
            check(assignments(source) == saved && source.type_workers == saved_groups, "caller proposal mutated");
            check(assignments(result.ordered_hint) == expected, "owned ordered proposal changed");
            ++controls;
        }
        std::cout << controls << " scripted budget/status/replay/profile/sparse-label controls passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
