#pragma once
#include "search.hpp"

namespace day_constructor {
struct TaskAssignment { int task, route, rank; };
struct WorkerProfile { Point start; int release; };

struct Bundles {
    const RoutingData& routing;
    std::vector<TaskAssignment> assignments;
    std::map<Key, std::vector<int>> members;
    std::vector<Key> task_bundle;
    std::map<Key, std::set<Key>> predecessors;
    std::vector<std::vector<Key>> routes;
    std::vector<int> early, late, capacities;
    std::vector<WorkerProfile> workers;
    std::map<int, std::array<Count, hours>> seed_supply;

    std::vector<Key> deadline_order(std::vector<Key> pending) const {
        std::vector<Key> result;
        Point point = *std::min_element(shed.begin(), shed.end());
        while (!pending.empty()) {
            std::optional<std::tuple<int, int, Key>> best;
            for (Key key : pending) {
                bool blocked = false;
                for (Key other : pending) blocked |= other.first == key.first && other.second < key.second;
                if (auto found = predecessors.find(key); found != predecessors.end())
                    for (Key predecessor : found->second) blocked |= std::find(pending.begin(), pending.end(), predecessor) != pending.end();
                if (blocked) continue;
                int limit = hours + 1;
                for (const auto& [other, ids] : members) {
                    if (other.first != key.first || other.second < key.second) continue;
                    for (int task : ids) if (auto found = routing.task_data.fixed_deadline.find(task); found != routing.task_data.fixed_deadline.end())
                        limit = std::min(limit, found->second);
                }
                const Point target = routing.task_data.tasks[members.at(key).front()].point;
                const auto score = std::tuple(limit, distance(point, target), key);
                if (!best || score < *best) best = score;
            }
            require(best.has_value(), "cyclic initial bundle precedence");
            const Key chosen = std::get<2>(*best);
            std::erase(pending, chosen);
            result.push_back(chosen);
            point = routing.task_data.tasks[members.at(chosen).front()].point;
        }
        return result;
    }

    static std::vector<TaskAssignment> solution_assignments(const RoutingData& data, const vrp::Solution& solution) {
        require(solution.isFeasible(), "bundle assembly requires a feasible routing proposal");
        std::vector<TaskAssignment> result;
        // The Python constructor numbers used routes here; it does not retain
        // a PyVRP vehicle's worker identity when coalescing and rematching.
        int route_index = 0;
        for (const auto& route : solution.routes()) {
            int rank = 0;
            for (const auto& activity : route.schedule()) {
                std::vector<int> ids;
                if (activity.isClient()) ids = data.client_tasks.at(activity.idx());
                else if (activity.isPickup() || activity.isDelivery()) {
                    const auto pair = data.shipment_tasks.at(activity.idx());
                    const int task = activity.isPickup() ? pair.first : pair.second;
                    if (task >= 0) ids.push_back(task);
                }
                for (int task : ids) result.push_back({task, route_index, rank++});
            }
            ++route_index;
        }
        return result;
    }

    Bundles(const RoutingData& data, const vrp::Solution& solution, bool split_resource_tasks = true)
        : Bundles(data, solution_assignments(data, solution), split_resource_tasks) {}

    Bundles(const RoutingData& data, std::vector<TaskAssignment> supplied, bool split_resource_tasks = true)
        : routing(data), assignments(std::move(supplied)) {
        const auto& tasks = data.task_data.tasks;
        const auto& deadline = data.task_data.fixed_deadline;
        const auto& problem = data.task_data.problem;
        int route_index = 0;
        std::vector<bool> seen(tasks.size());
        for (const auto& value : assignments) {
            require(value.task >= 0 && value.task < int(tasks.size()) && !seen[value.task], "invalid generated task partition");
            require(value.route >= 0 && value.rank >= 0, "negative generated route or rank");
            seen[value.task] = true;
            route_index = std::max(route_index, value.route + 1);
        }
        require(std::ranges::all_of(seen, [](bool value) { return value; }), "incomplete generated task partition");
        auto windows = data.task_data;
        windows.task_windows();  // repair windows omit assigned output deadlines
        for (const auto& [task, ready] : data.late_seeds) windows.early[task] = std::max(windows.early[task], ready);
        early = windows.early; late = windows.late;
        std::set<int> resource_tasks, split;
        for (const auto& [source, target] : data.resource_pairs) { resource_tasks.insert(source); resource_tasks.insert(target); }
        for (const auto& [task, hour] : deadline) split.insert(task);
        if (split_resource_tasks) split.insert(resource_tasks.begin(), resource_tasks.end());
        std::map<int, std::vector<int>> by_pattern, protected_group;
        for (const auto& task : tasks) by_pattern[task.pattern].push_back(task.id);
        for (const auto& task : tasks) {
            if (!deadline.contains(task.id) || task.output < 5 || task.output > 7) continue;
            std::vector<int> chain;
            for (int id = task.id; id >= 0 && tasks[id].pattern == task.pattern; id = tasks[id].predecessor) chain.push_back(id);
            std::reverse(chain.begin(), chain.end());
            for (int id : chain) protected_group[id] = chain;
        }
        for (const auto& [target, ready] : data.late_inputs) {
            if (tasks[target].op != kag::OP_PLACE) continue;
            std::vector<int> group{target};
            for (int id : by_pattern.at(tasks[target].pattern)) {
                if (id <= target) continue;
                if (tasks[id].op != kag::OP_FEED && tasks[id].op != kag::OP_CARE) break;
                group.push_back(id);
            }
            for (int id : group) protected_group[id] = group;
        }
        task_bundle.assign(tasks.size(), {-1, -1});
        for (const auto& [pattern, ids] : by_pattern) {
            int part = 0;
            std::vector<int> segment;
            std::set<std::vector<int>> emitted;
            auto emit = [&](const std::vector<int>& values) {
                const Key key{pattern, part++};
                members[key] = values;
                for (int task : values) task_bundle[task] = key;
            };
            auto flush = [&] { if (!segment.empty()) { emit(segment); segment.clear(); } };
            for (int task : ids) {
                if (data.late_seeds.contains(task)) flush();
                if (auto found = protected_group.find(task); found != protected_group.end()) {
                    flush();
                    if (!emitted.insert(found->second).second) continue;
                    emit(found->second);
                } else if (split.contains(task)) { flush(); emit({task}); }
                else segment.push_back(task);
            }
            flush();
        }
        std::map<Key, std::vector<TaskAssignment>> grouped;
        for (const auto& assignment : assignments) grouped[task_bundle.at(assignment.task)].push_back(assignment);
        std::vector<std::vector<std::pair<int, Key>>> ranked(route_index);
        for (const auto& [bundle, values] : grouped) {
            std::vector<int> input_routes, resource_routes;
            std::vector<std::pair<int, int>> outputs;
            std::map<int, int> counts;
            for (const auto& value : values) {
                if (auto found = deadline.find(value.task); found != deadline.end()) outputs.emplace_back(found->second, value.route);
                if (tasks[value.task].input >= 0) input_routes.push_back(value.route);
                if (resource_tasks.contains(value.task)) resource_routes.push_back(value.route);
                ++counts[value.route];
            }
            int owner = -1;
            if (!resource_routes.empty()) owner = resource_routes.front();
            else if (!outputs.empty()) owner = std::min_element(outputs.begin(), outputs.end())->second;
            else if (!input_routes.empty()) owner = input_routes.front();
            else {
                int most = -1;
                for (const auto& [worker, count] : counts) if (count > most) { most = count; owner = worker; }
            }
            int rank = std::numeric_limits<int>::max();
            for (const auto& value : values) if (value.route == owner) rank = std::min(rank, value.rank);
            ranked.at(owner).emplace_back(rank, bundle);
        }
        // Preserve the reference constructor's last-source assignment for a
        // repeated target bundle; exact scheduling owns all tile dependencies.
        for (const auto& [source, target] : data.resource_pairs) predecessors[task_bundle.at(target)] = {task_bundle.at(source)};
        for (auto& values : ranked) {
            if (values.empty()) continue;
            std::sort(values.begin(), values.end());
            std::vector<Key> route;
            for (const auto& [rank, key] : values) route.push_back(key);
            routes.push_back(deadline_order(route));
        }
        std::array<int, 4> occupancy{1, 0, 0, 0};
        for (int worker = 0; worker < problem.worker_count; ++worker) {
            const int index = worker == 0 ? 0 : int(std::min_element(occupancy.begin(), occupancy.end()) - occupancy.begin());
            if (worker) ++occupancy[index];
            const int release = worker == 0 ? 0 : data.task_data.hires[worker - 1] + 1;
            workers.push_back({shed[index], release});
            capacities.push_back(hours - release);
        }
        for (int crop = 0; crop < crops; ++crop) {
            Count demand = 0;
            for (const auto& task : tasks) demand += task.crop == crop;
            if (demand <= problem.start.seeds[crop]) continue;
            auto& supply = seed_supply[crop];
            supply.fill(problem.start.seeds[crop]);
            for (const auto& event : problem.market_plan)
                if (event.market_op == kag::M_BUY_SEED && event.item == crop)
                    for (int hour = event.hour + 1; hour < hours; ++hour) supply[hour] += event.quantity;
        }
    }
};
}  // namespace day_constructor
