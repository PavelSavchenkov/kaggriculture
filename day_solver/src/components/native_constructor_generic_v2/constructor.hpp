#pragma once
#include "../native_constructor_internal/export.hpp"

namespace day_constructor {
struct ConstructorOptions {
    Options routing;
    RepairOptions repair;
    int iterations = 5000;
    double seconds = 45;
    bool split_resource_tasks = true, use_all_workers = true;
    bool coalesce_patterns = true;
    ConstructorOptions() {
        routing.fixed_cost = 0; routing.seed = 1;
        routing.include_all_tasks = routing.worker_profile_fleet = routing.separate_delivery_routes = true;
        repair.seconds = 30;
    }
};
struct RouteSummary {
    Count duration, distance, start, end;
    std::string vehicle;
};
struct ConstructorResult {
    std::optional<RouteProposal> proposal;
    std::vector<RouteSummary> route_stats;
    std::vector<Key> resource_pairs;
    std::map<int, int> late_inputs, late_seeds;
    std::optional<Count> cost;
    int iterations = 0;
    double seconds = 0;
};

inline RouteProposal export_generic(const RoutingData& routing, const vrp::Solution& solution, bool allow_infeasible = false) {
    auto assignments = Bundles::solution_assignments(routing, solution, allow_infeasible);
    const auto& tasks = routing.task_data.tasks;
    const int workers = routing.task_data.problem.worker_count;
    require(int(solution.numRoutes()) <= workers, "too many generic worker routes");
    // Restore tile order in the original route slots. Moving the slots or
    // coalescing whole bundles would change the reference fallback proposal.
    std::map<std::pair<int, Point>, std::vector<size_t>> positions;
    std::vector<bool> seen(tasks.size());
    for (size_t index = 0; index < assignments.size(); ++index) {
        const auto& value = assignments[index];
        require(value.task >= 0 && value.task < int(tasks.size()) && !seen[value.task], "invalid generic task partition");
        seen[value.task] = true;
        positions[{value.route, tasks[value.task].point}].push_back(index);
    }
    if (routing.options.include_all_tasks)
        require(std::ranges::all_of(seen, [](bool value) { return value; }), "incomplete generic task partition");
    for (const auto& [key, indices] : positions) {
        std::vector<int> ordered;
        for (size_t index : indices) ordered.push_back(assignments[index].task);
        std::sort(ordered.begin(), ordered.end());
        for (size_t offset = 0; offset < indices.size(); ++offset) assignments[indices[offset]].task = ordered[offset];
    }
    RouteProposal result{};
    result.type_workers.emplace_back(workers);
    std::iota(result.type_workers.back().begin(), result.type_workers.back().end(), 0);
    for (const auto& value : assignments) result.assignments.push_back({value.task, value.route, value.rank, 0});
    return result;
}

inline ConstructorResult construct(const ds::DayProblem& problem, ConstructorOptions options) {
    require(options.iterations >= 0 && std::isfinite(options.seconds) && options.seconds >= 0, "invalid constructor budget");
    require(!options.coalesce_patterns || !options.routing.route_segments, "coalescing and route segments are alternatives");
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    const RoutingData routing(problem, options.routing);
    const auto data = routing.data();
    Search search(data, options.routing.seed);
    SearchOptions search_options;
    search_options.iterations = options.iterations > 0 ? options.iterations : std::numeric_limits<int>::max() - 1;
    // Iteration and time limits both apply. Include setup and the initial
    // local search in the budget; a search iteration is not interruptible.
    const auto stop = [&] { return elapsed() >= options.seconds; };
    const auto solution = search.run(search_options, {}, stop);
    ConstructorResult result;
    result.iterations = search.iterations_run;
    if (!solution->isFeasible()) { result.seconds = elapsed(); return result; }
    const vrp::CostEvaluator evaluator(std::vector<double>(data.numLoadDimensions(), 0), 0, 0);
    result.cost = Count(evaluator.cost(*solution));
    for (const auto& route : solution->routes()) result.route_stats.push_back({
        Count(route.duration()), Count(route.distance()), Count(route.startTime()), Count(route.endTime()), data.vehicleType(route.vehicleType()).name});
    result.resource_pairs = routing.resource_pairs;
    result.late_inputs = routing.late_inputs; result.late_seeds = routing.late_seeds;
    if (!options.coalesce_patterns) {
        result.proposal = export_generic(routing, *solution);
        result.seconds = elapsed();
        return result;
    }
    const Bundles bundles(routing, *solution, options.split_resource_tasks);
    options.repair.target_routes = options.use_all_workers ? problem.worker_count : 0;
    options.repair.separate_output_routes = options.routing.separate_delivery_routes;
    result.proposal = repair_and_export(bundles, options.repair, [&] { return std::max(0.0, options.seconds - elapsed()); });
    result.seconds = elapsed();
    return result;
}

// Internal proposals are generated by construct()/our semantic repair. This
// overload is not an original-route input to the public scheduling interface.
inline ConstructorResult repair_generated(const ds::DayProblem& problem, ConstructorOptions options,
                                          const std::vector<ProposalTask>& proposal) {
    require(options.coalesce_patterns, "internal bundle repair requires coalescing");
    require(!options.routing.route_segments, "coalescing and route segments are alternatives");
    const auto started = std::chrono::steady_clock::now();
    const RoutingData routing(problem, options.routing);
    std::map<Key, int> indices;
    for (const auto& task : proposal) {
        require(task.worker >= 0 && task.type >= 0, "negative generated route label");
        indices[{task.type, task.worker}] = 0;
    }
    int index = 0;
    for (auto& [key, value] : indices) value = index++;
    require(int(indices.size()) <= problem.worker_count, "too many generated worker routes");
    std::vector<TaskAssignment> assignments;
    for (const auto& task : proposal) assignments.push_back({task.task, indices.at({task.type, task.worker}), task.rank});
    const Bundles bundles(routing, std::move(assignments), options.split_resource_tasks);
    RepairContext context(bundles, true);
    options.repair.target_routes = options.use_all_workers ? problem.worker_count : 0;
    options.repair.separate_output_routes = options.routing.separate_delivery_routes;
    RouteProposal repaired;
    repaired.repaired = Repair(context, options.repair).run(context.initial, [&] {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() >= options.seconds;
    });
    export_workers(context, repaired);
    ConstructorResult result;
    result.proposal = std::move(repaired);
    result.resource_pairs = routing.resource_pairs;
    result.late_inputs = routing.late_inputs; result.late_seeds = routing.late_seeds;
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return result;
}
}  // namespace day_constructor
