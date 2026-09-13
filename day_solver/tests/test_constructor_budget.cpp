#include "../src/components/native_constructor_generic_v2/constructor.hpp"
#include "../src/components/native_constructor_generic_v2/candidates.hpp"
#include "problem_json.hpp"
#include <iostream>

int main(int argc, char** argv) {
    day_constructor::require(argc == 3, "supply budget and feasible constructor fixtures");
    const auto problem = day_solver::load_problem_json(argv[1]);
    day_constructor::ConstructorOptions options;
    options.iterations = 1000;
    options.seconds = 0;
    const auto result = day_constructor::construct(problem, options);
    day_constructor::require(result.iterations == 0, "positive iteration count ignored exhausted time budget");
    const day_constructor::ConstructorCandidates bounded(problem, options);
    day_constructor::require(bounded.iterations == 0, "checkpoint search ignored exhausted time budget");
    const day_constructor::TaskData data(problem);
    std::vector<day_constructor::ProposalTask> route;
    for (const auto& task : data.tasks) route.push_back({task.id, 0, task.id, 0});
    const auto repaired = day_constructor::repair_generated(problem, options, route);
    day_constructor::require(repaired.proposal && repaired.proposal->repaired.stats.forced_splits == 0,
        "exhausted repair budget still searched for extra worker routes");
    options.iterations = 512;
    options.seconds = 10;
    options.coalesce_patterns = false;
    const auto feasible_problem = day_solver::load_problem_json(argv[2]);
    const auto reference = day_constructor::construct(feasible_problem, options);
    const day_constructor::ConstructorCandidates candidates(feasible_problem, options);
    day_constructor::require(reference.iterations == 512 && candidates.iterations == 512,
        "fixed-iteration constructor control timed out");
    day_constructor::require(bool(reference.proposal) && candidates.size() > 0,
        "constructor control did not produce a feasible partition");
    const auto first = candidates.prepare(0, 0);
    auto assignments = [](const auto& proposal) {
        std::vector<std::array<int, 4>> result;
        for (const auto& row : proposal.assignments) result.push_back({row.task, row.worker, row.rank, row.type});
        return result;
    };
    day_constructor::require(first.proposal && first.cost == reference.cost &&
        first.proposal->type_workers == reference.proposal->type_workers &&
        assignments(*first.proposal) == assignments(*reference.proposal),
        "checkpoint collection changed the normal final routing proposal");
    std::cout << "Constructor budgets stop search; checkpoint collection preserves the final proposal.\n";
}
