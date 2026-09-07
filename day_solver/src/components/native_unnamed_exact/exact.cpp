#include "exact.hpp"
#include "exact_model.hpp"

namespace day_native::unnamed {
static Result run(const StaticData& data, const HintOptions& options, const SolveOptions& settings, int tuning) {
    const auto& problem = data.problem;
    require(std::isfinite(settings.seconds) && settings.seconds > 0 && settings.workers > 0, "positive search budget and worker count required");
    for (int route : options.free_routes) require(route >= 0, "negative free task/route");
    for (int task : options.free_tasks) require(task >= 0, "negative free task/route");
    const auto started = std::chrono::steady_clock::now();
    ExactModel exact(data, options);
    NativeHints hints{exact, options, {}, {}};
    hints.apply();
    if (settings.earliest) exact.model.Minimize(LinearExpr::Sum(exact.events));
    const auto& proto = exact.model.Build();
    const auto validation = sat::ValidateCpModel(proto);
    require(validation.empty(), validation);
    Result result;
    result.tasks = int(exact.tasks.size()); result.workers = problem.worker_count;
    result.variables = proto.variables_size(); result.constraints = proto.constraints_size();
    result.pruned_task_variables = exact.pruned_task_variables;
    result.build_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result.restrictions = hints.restrictions;
    if (settings.include_model) result.model = proto;
    if (settings.build_only) return result;
    sat::SatParameters parameters;
    parameters.set_max_time_in_seconds(settings.seconds);
    parameters.set_num_search_workers(settings.workers);
    parameters.set_random_seed(0);
    require(tuning >= 0 && tuning <= 2, "invalid exact tuning");
    parameters.set_symmetry_level(tuning == 0 ? 3 : 0);
    if (tuning > 0) parameters.set_cp_model_probing_level(0);
    if (tuning == 2) parameters.set_max_presolve_iterations(1);
    parameters.set_log_search_progress(settings.log_search);
    sat::Model solver;
    solver.Add(sat::NewSatParameters(parameters));
    const auto response = sat::SolveCpModel(proto, &solver);
    result.solved = true; result.status = response.status(); result.solver_seconds = response.wall_time();
    result.branches = response.num_branches(); result.conflicts = response.num_conflicts();
    result.cores = hints.result_cores(response);
    if (response.status() == sat::OPTIMAL || response.status() == sat::FEASIBLE) {
        require(sat::SolutionIsFeasible(proto, {response.solution().data(), std::size_t(response.solution_size())}), "invalid CP assignment");
        result.schedule = exact.materialize(response);
        const auto replay = ds::replay_schedule(problem, *result.schedule);
        const bool accepted = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied;
        result.replay = ReplayCheck{accepted, replay.candidate.replay.strict_valid, replay.requirements_satisfied, replay.invariants_satisfied, replay.errors};
    }
    return result;
}
struct Session::Impl {
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    const StaticData data;
    const double seconds;
    explicit Impl(const ds::DayProblem& problem) : data(problem),
        seconds(std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count()) {}
};
Session::Session(const ds::DayProblem& problem) : impl_(std::make_unique<const Impl>(problem)) {}
Session::~Session() = default;
double Session::preparation_seconds() const { return impl_->seconds; }
Result Session::solve(const HintOptions& hints, const SolveOptions& options, int tuning) const {
    return run(impl_->data, hints, options, tuning);
}
Result solve(const ds::DayProblem& problem, const HintOptions& hints, const SolveOptions& options, int tuning) {
    Session session(problem);
    auto result = session.solve(hints, options, tuning);
    result.build_seconds += session.preparation_seconds();
    return result;
}
} // namespace day_native::unnamed
