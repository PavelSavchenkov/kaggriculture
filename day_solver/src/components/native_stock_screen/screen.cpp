#include "screen.hpp"
#include "data.hpp"
#include "model.hpp"
#include "ortools/sat/cp_model_checker.h"

namespace day_native::stock_screen {
Result solve(const ds::DayProblem& problem, const InternalHint& hint, const SolveOptions& settings, int tuning) {
    require(std::isfinite(settings.seconds) && settings.seconds > 0 && settings.workers > 0, "positive search limits required");
    const auto& options = settings.model;
    require(!(options.ignore_precedence && options.soft_precedence), "precedence cannot be both ignored and softened");
    const auto started = std::chrono::steady_clock::now();
    const ScreenData data(problem, hint, options);
    Result result;
    if (settings.include_data) result.data = DataSnapshot{data.tasks, data.early, data.late, data.deadlines, data.owner, data.route_ids,
        data.worker_group, data.requirements, data.deliveries, data.net, data.workers, data.profiles, data.groups};
    ScreenModel screen(data);
    const auto& proto = screen.model.Build();
    const auto validation = sat::ValidateCpModel(proto);
    require(validation.empty(), validation);
    result.build_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result.variables = proto.variables_size(); result.constraints = proto.constraints_size();
    result.routes = int(data.route_ids.size());
    result.initial_spawns_coupled = options.dynamic && std::count(data.hires.begin(), data.hires.end(), 0) > 0;
    result.hire_checkpoints_coupled = options.dynamic;
    result.pickup_requirements = screen.pickup_requirements;
    if (settings.include_model) result.model = proto;
    if (!settings.build_only) {
        sat::SatParameters parameters;
        parameters.set_max_time_in_seconds(settings.seconds);
        parameters.set_num_search_workers(settings.workers);
        parameters.set_random_seed(settings.seed);
        require(tuning >= 0 && tuning <= 2, "invalid screen tuning");
        if (tuning > 0) {
            parameters.set_symmetry_level(0);
            parameters.set_cp_model_probing_level(0);
        }
        if (tuning == 2) parameters.set_max_presolve_iterations(1);
        sat::Model solver;
        solver.Add(sat::NewSatParameters(parameters));
        const auto response = sat::SolveCpModel(proto, &solver);
        result.solved = true; result.status = response.status(); result.solver_seconds = response.wall_time();
        result.branches = response.num_branches(); result.conflicts = response.num_conflicts();
        if (response.status() == sat::OPTIMAL || response.status() == sat::FEASIBLE) {
            require(sat::SolutionIsFeasible(screen.model.Build(), {response.solution().data(), std::size_t(response.solution_size())}), "invalid coarse CP assignment");
            auto value = [&](auto variable) { return sat::SolutionIntegerValue(response, variable); };
            const auto selected_profiles = screen.checkpoints ? screen.checkpoints->selected(response) : data.workers;
            if (screen.checkpoints) result.worker_start_profiles = selected_profiles;
            for (const auto& [previous, next, violated] : screen.cross_violations)
                if (sat::SolutionBooleanValue(response, violated)) result.violations.push_back({previous, next, data.owner[previous], data.owner[next],
                    value(screen.task_time[previous]), value(screen.task_time[next])});
            for (const auto& [item, deadline, task, deficit] : screen.deficits)
                if (value(deficit)) result.deficits.push_back({item, deadline, task >= 0 ? std::optional<int>(task) : std::nullopt, value(deficit)});
            for (const auto& task : data.tasks) if (data.delivery_tasks.contains(task.id)) {
                Delivery delivery{task.id, task.output, data.owner[task.id], value(screen.task_time[task.id]), task.quantity, 0, {}};
                for (const auto& [key, units] : screen.delivered_units) if (std::get<0>(key) == task.id) delivery.delivered += value(units);
                for (int deadline : data.deadlines) {
                    Count quantity = 0;
                    for (const auto& [key, units] : screen.delivered_units)
                        if (std::get<0>(key) == task.id && std::get<1>(key) <= deadline) quantity += value(units);
                    delivery.delivered_by[deadline] = quantity;
                }
                result.deliveries.push_back(std::move(delivery));
            }
            std::vector<std::vector<int>> groups;
            std::vector<int> group_of;
            if (options.fixed_profile) groups = data.source_groups;
            else {
                std::vector<Profile> unique;
                for (int worker = 0; worker < int(selected_profiles.size()); ++worker) {
                    const auto found = std::find(unique.begin(), unique.end(), selected_profiles[worker]);
                    const int group = found - unique.begin();
                    if (found == unique.end()) { unique.push_back(selected_profiles[worker]); groups.emplace_back(); }
                    groups[group].push_back(worker); group_of.push_back(group);
                }
            }
            InternalHint proposal;
            proposal.type_workers = std::move(groups);
            for (const auto& task : data.tasks) {
                const int route = data.owner[task.id], worker = value(screen.concrete_worker[route]);
                const int type = options.fixed_profile ? data.source_type.at(data.route_ids[route]) : group_of[worker];
                proposal.assignments.push_back({task.id, worker, int(value(screen.task_time[task.id])), type, {}});
            }
            result.hint = std::move(proposal);
        }
    }
    result.total_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return result;
}
} // namespace day_native::stock_screen
