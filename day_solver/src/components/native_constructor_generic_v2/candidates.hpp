#pragma once
#include "constructor.hpp"

namespace day_constructor {
// Optional cold-search experiment. Retain a few distinct best-so-far routes
// without changing the bundled constructor types or the normal construct().
class ConstructorCandidates {
    using Clock = std::chrono::steady_clock;
    const Clock::time_point started_ = Clock::now();
    ConstructorOptions options_;
    RoutingData routing_;
    vrp::ProblemData data_;
    struct Entry { int iteration; SolutionPtr solution; int diversity = 0; };
    std::vector<Entry> entries_;

    static ConstructorOptions checked(ConstructorOptions options) {
        require(options.iterations >= 0 && std::isfinite(options.seconds) && options.seconds >= 0,
                "invalid constructor budget");
        require(!options.coalesce_patterns || !options.routing.route_segments,
                "coalescing and route segments are alternatives");
        return options;
    }
    double elapsed() const {
        return std::chrono::duration<double>(Clock::now() - started_).count();
    }
    void remember(int iteration, const vrp::Solution& solution) {
        if (!solution.isFeasible()) return;
        if (std::ranges::any_of(entries_, [&](const auto& entry) { return *entry.solution == solution; })) return;
        entries_.push_back({iteration, std::make_shared<vrp::Solution>(solution)});
    }
    void diversify() {
        std::vector<std::vector<int>> successors;
        for (const auto& entry : entries_) {
            auto assignments = Bundles::solution_assignments(routing_, *entry.solution);
            std::sort(assignments.begin(), assignments.end(), [](const auto& a, const auto& b) {
                return std::tie(a.route, a.rank) < std::tie(b.route, b.rank);
            });
            auto& next = successors.emplace_back(routing_.task_data.tasks.size(), -1);
            for (size_t index = 1; index < assignments.size(); ++index)
                if (assignments[index - 1].route == assignments[index].route)
                    next[assignments[index - 1].task] = assignments[index].task;
        }
        // After the normal final result, prefer different task adjacencies.
        // Nearby iterations often produce nearly identical uncompletable routes.
        for (size_t index = 1; index < entries_.size(); ++index) {
            size_t best = index;
            int best_distance = -1;
            for (size_t candidate = index; candidate < entries_.size(); ++candidate) {
                int nearest = int(routing_.task_data.tasks.size());
                for (size_t selected = 0; selected < index; ++selected) {
                    int distance = 0;
                    for (size_t task = 0; task < successors[candidate].size(); ++task)
                        distance += successors[candidate][task] != successors[selected][task];
                    nearest = std::min(nearest, distance);
                }
                if (nearest > best_distance) { best = candidate; best_distance = nearest; }
            }
            std::swap(entries_[index], entries_[best]);
            std::swap(successors[index], successors[best]);
            entries_[index].diversity = best_distance;
        }
    }
public:
    int iterations = 0;
    double search_seconds = 0;

    ConstructorCandidates(const ds::DayProblem& problem, ConstructorOptions options)
        : options_(checked(std::move(options))), routing_(problem, options_.routing), data_(routing_.data()) {
        Search search(data_, options_.routing.seed);
        SearchOptions limits;
        limits.iterations = options_.iterations > 0 ? options_.iterations : std::numeric_limits<int>::max() - 1;
        const auto solution = search.run(limits,
            [&](int iteration, const auto&, const auto&, const auto& best, const auto&, const auto&, const auto&) {
                // A fixed geometric family, independent of the replay or its routes.
                if (iteration >= 256 && iteration <= 4096 && (iteration & (iteration - 1)) == 0)
                    remember(iteration, best);
            }, [&] { return elapsed() >= options_.seconds; });
        iterations = search.iterations_run;
        // Preserve the normal result first, then distinct checkpoints newest first.
        std::erase_if(entries_, [&](const auto& entry) { return *entry.solution == *solution; });
        remember(iterations, *solution);
        std::reverse(entries_.begin(), entries_.end());
        diversify();
        search_seconds = elapsed();
    }

    size_t size() const { return entries_.size(); }
    int diversity(size_t index) const { return entries_.at(index).diversity; }

    ConstructorResult prepare(size_t index, double seconds) const {
        require(std::isfinite(seconds) && seconds >= 0, "invalid candidate repair budget");
        const auto started = Clock::now();
        const auto& entry = entries_.at(index);
        const auto& solution = *entry.solution;
        ConstructorResult result;
        result.iterations = entry.iteration;
        const vrp::CostEvaluator evaluator(std::vector<double>(data_.numLoadDimensions(), 0), 0, 0);
        result.cost = Count(evaluator.cost(solution));
        for (const auto& route : solution.routes()) result.route_stats.push_back({
            Count(route.duration()), Count(route.distance()), Count(route.startTime()), Count(route.endTime()),
            data_.vehicleType(route.vehicleType()).name});
        result.resource_pairs = routing_.resource_pairs;
        result.late_inputs = routing_.late_inputs;
        result.late_seeds = routing_.late_seeds;
        if (!options_.coalesce_patterns) result.proposal = export_generic(routing_, solution);
        else {
            const Bundles bundles(routing_, solution, options_.split_resource_tasks);
            auto repair = options_.repair;
            repair.target_routes = options_.use_all_workers ? routing_.task_data.problem.worker_count : 0;
            repair.separate_output_routes = options_.routing.separate_delivery_routes;
            repair.seconds = std::min(repair.seconds, seconds);
            result.proposal = repair_and_export(bundles, repair, [&] {
                return std::max(0.0, seconds - std::chrono::duration<double>(Clock::now() - started).count());
            });
        }
        result.seconds = std::chrono::duration<double>(Clock::now() - started).count();
        return result;
    }
};
}  // namespace day_constructor
