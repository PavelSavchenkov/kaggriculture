#include "completion.hpp"
#include "../native_semantic_candidates/candidates.hpp"
#include <chrono>
#include <cmath>

namespace day_native::early {
bool Result::accepted() const {
    return exact && exact->solved &&
        (exact->status == operations_research::sat::OPTIMAL || exact->status == operations_research::sat::FEASIBLE) &&
        exact->schedule && exact->replay && exact->replay->accepted;
}

Backends native_backends() {
    return {screen::solve, exact::solve, [] {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }};
}

Result complete(const day_solver::DayProblem& problem, const InternalHint& proposal, const Options& options, const Backends& backends) {
    for (double seconds : {options.seconds, options.screen_seconds, options.exact_seconds})
        if (!std::isfinite(seconds) || seconds < 0) throw std::runtime_error("nonnegative finite completion budgets required");
    const auto started = backends.now();
    auto remaining = [&] { return options.seconds - (backends.now() - started); };
    Result result;
    day_semantic::Hint source;
    source.type_workers = proposal.type_workers.value_or(std::vector<std::vector<int>>{});
    for (const auto& row : proposal.assignments)
        source.assignments.push_back({row.task, row.worker, required(row.hour, "hour"), row.type.value_or(0)});
    const auto ordered = day_semantic::write_hint(day_semantic::read_routes(source), source);
    result.ordered_hint.type_workers = ordered.type_workers;
    for (const auto& row : ordered.assignments)
        result.ordered_hint.assignments.push_back({row.task, row.worker, row.rank, row.type, {}});
    const double screen_budget = std::min(options.screen_seconds, remaining());
    if (screen_budget > 0) {
        screen::SolveOptions settings;
        settings.seconds = screen_budget; settings.workers = 1; settings.seed = 0;
        settings.model.dynamic = true; settings.model.fixed_order = true;
        settings.model.fixed_profile = options.fix_source_profiles;
        result.coarse = backends.screen(problem, result.ordered_hint, settings);
        const auto& coarse = *result.coarse;
        const bool incumbent = coarse.solved &&
            (coarse.status == operations_research::sat::OPTIMAL || coarse.status == operations_research::sat::FEASIBLE) && coarse.hint;
        const double exact_budget = std::min(options.exact_seconds, remaining());
        if (incumbent && exact_budget > 0) {
            if (!coarse.deficits.empty() || !coarse.violations.empty()) throw std::runtime_error("hard coarse solve returned penalties");
            exact::HintOptions hints;
            hints.documents.emplace(HintKind::fixed_partial, *coarse.hint);
            exact::SolveOptions settings;
            settings.seconds = exact_budget; settings.workers = 1;
            result.exact = backends.exact(problem, hints, settings);
        }
    }
    result.seconds = backends.now() - started;
    return result;
}
} // namespace day_native::early
