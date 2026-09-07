#include "day_solver/scheduler.hpp"
#include "problem_validation.hpp"
#include <stdexcept>

namespace day_scheduler {
void prepare_problem(day_solver::DayProblem& problem) {
    using namespace day_solver;
    if (problem.format_version != FORMAT_VERSION)
        throw std::runtime_error("prepare_problem requires public format v3");
    problem.required_outcomes.clear();
    problem.allowed_acquisitions.clear();
    for (int item = 0; item < kag::N_ITEMS; ++item)
        problem.required_outcomes.push_back({
            {OutcomeMetric::END_SHED, static_cast<int16_t>(item), NO_TILE, -1},
            problem.end_shed[item], problem.end_shed[item]});
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        problem.required_outcomes.push_back({
            {OutcomeMetric::END_SEEDS, static_cast<int16_t>(crop), NO_TILE, -1},
            problem.end_seeds[crop], problem.end_seeds[crop]});
    for (auto& end : problem.required_end_tiles) {
        if (!end.exact_state) continue;
        end.kind = end.exact_state->kind;
        end.item = end.kind == ManagedTileKind::CROP ? end.exact_state->crop
            : animal_structure(end.kind) ? end.exact_state->animal : NO_SUBJECT;
    }
    for (const auto& event : problem.market_plan) {
        if (event.market_op != kag::M_BUY_SEED && event.market_op != kag::M_BUY_PRODUCT &&
            event.market_op != kag::M_BUY_ANIMAL) continue;
        AllowedAcquisition acquisition;
        acquisition.market_op = event.market_op;
        acquisition.item = event.item;
        acquisition.lower = acquisition.upper = event.quantity;
        acquisition.first_hour = acquisition.last_hour = event.hour;
        problem.allowed_acquisitions.push_back(std::move(acquisition));
    }
    const auto issues = validate_problem(problem);
    if (!issues.empty()) throw std::runtime_error(issues.front().path + ": " + issues.front().message);
}
}
