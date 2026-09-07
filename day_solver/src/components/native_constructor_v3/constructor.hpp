#pragma once
#include "export.hpp"

namespace day_constructor {
struct ConstructorOptions {
    Options routing;
    RepairOptions repair;
    int iterations = 5000;
    double seconds = 45;
    bool split_resource_tasks = true, use_all_workers = true;
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
    Count cost = 0;
    int iterations = 0;
    double seconds = 0;
};

inline ConstructorResult construct(const ds::DayProblem& problem, ConstructorOptions options) {
    require(options.iterations >= 0 && std::isfinite(options.seconds) && options.seconds >= 0, "invalid constructor budget");
    require(!options.routing.route_segments, "coalescing and route segments are alternatives");
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    const RoutingData routing(problem, options.routing);
    const auto data = routing.data();
    Search search(data, options.routing.seed);
    SearchOptions search_options;
    search_options.iterations = options.iterations > 0 ? options.iterations : std::numeric_limits<int>::max() - 1;
    std::optional<std::chrono::steady_clock::time_point> search_started;
    std::function<bool()> stop;
    if (options.iterations == 0) stop = [&] {
        // PyVRP MaxRuntime starts on its first call, after initial search.
        if (!search_started) search_started = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - *search_started).count() > options.seconds;
    };
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
    const Bundles bundles(routing, *solution, options.split_resource_tasks);
    options.repair.target_routes = options.use_all_workers ? problem.worker_count : 0;
    options.repair.separate_output_routes = options.routing.separate_delivery_routes;
    result.proposal = repair_and_export(bundles, options.repair, [&] { return std::max(0.0, options.seconds - elapsed()); });
    result.seconds = elapsed();
    return result;
}
}  // namespace day_constructor
