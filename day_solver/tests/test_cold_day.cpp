#include "day_solver/scheduler.hpp"
#include "problem_json.hpp"
#include "replay.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc < 2) throw std::runtime_error("supply cold day contracts");
    const bool search_regressions = std::string(argv[1]) == "--search-regressions";
    for (int index = search_regressions ? 2 : 1; index < argc; ++index) {
        const auto problem = day_solver::load_problem_json(argv[index]);
        const bool fast = search_regressions && index == 2;
        const double seconds = search_regressions ? (fast ? 0.5 : 4.0) : 12.0;
        const auto policy = fast ? day_scheduler::Search::RegretFast : day_scheduler::Search::Portfolio;
        const auto result = day_scheduler::solve(problem, {seconds, 1, policy});
        if (!result.schedule) {
            for (const auto& stage : result.stages)
                std::cerr << stage.name << '\t' << stage.status << '\t' << stage.seconds << '\n';
            throw std::runtime_error(std::string(argv[index]) + ": cold contract returned UNKNOWN");
        }
        if (search_regressions && result.seconds > seconds)
            throw std::runtime_error("search regression exceeded its time limit");
        const auto replay = day_solver::replay_schedule(problem, *result.schedule);
        if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied ||
            !replay.invariants_satisfied || !replay.errors.empty())
            throw std::runtime_error("cold schedule failed strict replay");
        std::cout << argv[index] << " strict " << result.seconds << std::endl;
    }
}
