#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include <iostream>
#include <stdexcept>

// No file argument: construct a tomato-planting day entirely through C++ types.
static day_solver::DayProblem tomato_day() {
    using namespace day_solver;
    DayProblem problem;
    problem.start.managed_tiles.push_back({4, 4, {}});
    ManagedTileState end;
    end.kind = ManagedTileKind::CROP;
    end.crop = kag::TOMATO;
    end.age_days = 1;
    EndTileRequirement successor;
    successor.tile = 0;
    successor.exact_state = end;
    problem.required_end_tiles.push_back(successor);
    problem.tile_work.push_back({0, {{kag::OP_PLANT, kag::TOMATO, 1, -1, 0},
                                     {kag::OP_WATER, -1, 1, -1, 0}}});
    problem.market_plan.push_back({0, 0, kag::M_BUY_SEED, kag::TOMATO, 1, 0});
    day_scheduler::prepare_problem(problem);
    return problem;
}

int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: day_solver_example [public-v3.json]");
        const auto problem = argc == 2 ? day_solver::load_problem_json(argv[1]) : tomato_day();
        const auto result = day_scheduler::solve(problem);
        if (!result.schedule) {
            std::cout << "UNKNOWN " << result.seconds << " s\n";
            return 1;
        }
        // Consume these 24 actions directly in your planning pipeline.
        const auto& schedule = *result.schedule;
        for (int hour = 0; hour < day_solver::HOURS; ++hour)
            std::cout << "hour " << hour << ": " << schedule[hour].n_units
                      << " active workers, " << schedule[hour].n_orders << " order slots\n";
        std::cout << "SCHEDULE " << result.seconds << " s\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
