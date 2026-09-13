#include "fast_routes.hpp"
#include "components/native_constructor_generic_v2/constructor.hpp"
#include "components/native_materialized_screen/screen.hpp"
#include "replay.hpp"

namespace day_scheduler {
Result fast_routes(const day_solver::DayProblem& problem, double seconds, int profile, int iterations) {
    namespace dc = day_constructor;
    namespace dn = day_native;
    namespace sat = operations_research::sat;
    if (!std::isfinite(seconds) || seconds < 0 || profile < 0 || profile > 3 || iterations < 1)
        throw std::runtime_error("invalid fast route settings");
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    Result result;
    auto finish = [&] { result.seconds = elapsed(); return std::move(result); };
    if (seconds == 0) return finish();
    dc::ConstructorOptions options;
    options.iterations = iterations;
    if (profile == 1) options.routing.shared_resource_flow = true;
    if (profile >= 2) options.routing.separate_delivery_routes = false;
    if (profile == 3) { options.routing.seed = 3; options.routing.pair_jitter = 200; }
    const dc::RoutingData routing(problem, options.routing);
    const auto data = routing.data();
    result.stages.push_back({"fast_routes/prepare", "BUILT", elapsed()});
    if (elapsed() >= seconds) return finish();
    const auto route_started = elapsed();
    dc::Search search(data, options.routing.seed);
    dc::SearchOptions search_options; search_options.iterations = iterations;
    const dc::vrp::CostEvaluator score(std::vector<double>(data.numLoadDimensions(), 100000), 100000, 100000);
    dc::SolutionPtr partial;
    search.run(search_options, [&](int, const auto&, const auto& candidate, const auto&, const auto&, const auto&, const auto&) {
        if (candidate.isComplete() && (!partial || score.penalisedCost(candidate) < score.penalisedCost(*partial)))
            partial = std::make_shared<dc::vrp::Solution>(candidate);
    }, [&] { return elapsed() >= seconds; });
    result.stages.push_back({"fast_routes/partition", partial ? "COMPLETE_PARTITION" : "UNKNOWN", elapsed() - route_started});
    if (!partial || elapsed() >= seconds) return finish();
    const auto raw = dc::export_generic(routing, *partial, true);
    options.seconds = options.repair.seconds = std::min(0.15, seconds - elapsed());
    options.repair.optimize_balance = true;
    const auto repaired = dc::repair_generated(problem, options, raw.assignments);
    result.stages.push_back({"fast_routes/repair", repaired.proposal ? "PROPOSAL" : "UNKNOWN", repaired.seconds});
    if (!repaired.proposal || elapsed() >= seconds) return finish();
    dn::InternalHint hint; hint.type_workers = repaired.proposal->type_workers;
    for (const auto& row : repaired.proposal->assignments)
        hint.assignments.push_back({row.task, row.worker, row.rank, row.type, {}});
    dn::screen::SolveOptions completion;
    completion.seconds = seconds - elapsed(); completion.workers = 1;
    completion.model.dynamic = completion.model.fixed_order = true;
    auto completed = dn::materialized_screen::solve(problem, hint, completion, 2);
    result.stages.push_back({"fast_routes/complete", sat::CpSolverStatus_Name(completed.status), completed.total_seconds});
    if (completed.schedule) {
        const auto replay = day_solver::replay_schedule(problem, *completed.schedule);
        if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied ||
            !replay.invariants_satisfied || !replay.errors.empty())
            throw std::runtime_error("fast route completion disagrees with strict replay");
        result.schedule = std::move(completed.schedule);
    }
    return finish();
}
}
