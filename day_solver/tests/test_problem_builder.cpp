#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* error) {
    if (!condition) throw std::runtime_error(error);
}

int main(int argc, char** argv) {
    require(argc > 1, "supply fixtures");
    for (int arg = 1; arg < argc; ++arg) {
        const auto parsed = day_solver::load_problem_json(argv[arg]);
        auto built = parsed;
        built.required_outcomes.clear();
        built.allowed_acquisitions.clear();
        for (auto& end : built.required_end_tiles) {
            end.kind = day_solver::ManagedTileKind::EMPTY;
            end.item = -1;
        }
        day_scheduler::prepare_problem(built);
        require(day_solver::serialize_problem_json(built) == day_solver::serialize_problem_json(parsed),
                "typed builder changed public requirements");
        require(built.required_outcomes.size() == parsed.required_outcomes.size(), "missing end bounds");
        require(built.allowed_acquisitions.size() == parsed.allowed_acquisitions.size(), "missing purchases");
        // Mimic an outer planner modifying both terminal and start inventory.
        ++built.start.shed[kag::WOOL];
        ++built.end_shed[kag::WOOL];
        day_scheduler::prepare_problem(built);
        const auto result = day_scheduler::solve(built);
        require(result.schedule.has_value(), "edited problem returned UNKNOWN");
        const auto replay = day_solver::replay_schedule(built, *result.schedule);
        require(replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied,
                "edited terminal inventory was not satisfied");
    }
    std::cout << "Typed construction and changed terminal inventories passed.\n";
}
