#include "day_solver/scheduler.hpp"
#include "problem_json.hpp"
#include "replay.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    require(argc > 1, "supply public v3 fixtures");
    for (int index = 1; index < argc; ++index) {
        const auto problem = day_solver::load_problem_json(argv[index]);
        const auto before = day_solver::serialize_problem_json(problem);
        const auto result = day_scheduler::solve(problem);
        require(result.schedule.has_value(), "fixture returned UNKNOWN");
        const auto replay = day_solver::replay_schedule(problem, *result.schedule);
        require(replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied,
                "public result failed replay");
        require(std::isfinite(result.seconds) && result.seconds >= 0, "invalid elapsed time");
        require(before == day_solver::serialize_problem_json(problem), "input was mutated");
        require(!day_scheduler::solve(problem, {0, 1}).schedule, "zero budget must return UNKNOWN");
        for (auto policy : {day_scheduler::Search::Regret, day_scheduler::Search::RegretDeferred,
                            day_scheduler::Search::RegretFast}) {
            require(!day_scheduler::solve(problem, {0, 1, policy}).schedule,
                    "zero fast budget must return UNKNOWN");
            const auto fast = day_scheduler::solve(problem, {2, 1, policy});
            if (fast.schedule) {
                const auto checked = day_solver::replay_schedule(problem, *fast.schedule);
                require(checked.candidate.replay.strict_valid && checked.requirements_satisfied && checked.invariants_satisfied,
                        "fast public result failed replay");
            }
            require(before == day_solver::serialize_problem_json(problem), "fast search mutated input");
        }
        bool rejected = false;
        try { day_scheduler::solve(problem, {1, 1, static_cast<day_scheduler::Search>(99)}); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "invalid search policy was accepted");
        rejected = false;
        try { day_scheduler::solve(problem, {std::numeric_limits<double>::quiet_NaN(), 1}); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "invalid budget was accepted");
        auto malformed = problem;
        malformed.format_version = 2;
        rejected = false;
        try { day_scheduler::solve(malformed); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "non-v3 input was accepted");
        std::cout << argv[index] << " strict " << result.seconds << '\n';
    }
}
