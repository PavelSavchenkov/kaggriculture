#pragma once

#include "task_data.hpp"
#include "ProblemData.h"
#include "vendor/rectangular_lsap.h"

namespace day_constructor {

struct Options {
    int fixed_cost = 1000, route_hours = 23, seed = 0, pair_jitter = 22;
    bool include_all_tasks = false, route_segments = false, worker_profile_fleet = false;
    bool shared_resource_flow = false, separate_delivery_routes = false, shared_seed_flow = false;
};

struct RoutingData {
    TaskData task_data;
    Options options;
    std::vector<int> input_items;
    std::array<Count, items> free_inputs{};
    std::map<int, int> input_ready, late_inputs, late_seeds;
    std::vector<std::pair<int, int>> resource_pairs, shipment_tasks;
    std::vector<std::vector<int>> client_tasks;
    std::vector<Point> points;
    std::vector<pyvrp::Location> locations;
    std::vector<pyvrp::Depot> depots;
    std::vector<pyvrp::VehicleType> vehicles;
    std::vector<pyvrp::Client> clients;
    std::vector<pyvrp::Shipment> shipments;
    std::vector<pyvrp::Matrix<pyvrp::Distance>> distances;
    std::vector<pyvrp::Matrix<pyvrp::Duration>> durations;

    explicit RoutingData(const ds::DayProblem& problem, Options supplied = {})
        : task_data(problem), options(supplied) {
        require(options.seed >= 0 && options.pair_jitter >= 0, "negative seed or pair jitter");
        require(options.fixed_cost >= 0 && options.route_hours >= 0, "negative routing cost or duration");
        std::set<int> item_set;
        for (const auto& task : task_data.tasks) if (task.input >= 0) item_set.insert(task.input);
        input_items.assign(item_set.begin(), item_set.end());
        prepare_inputs();
        prepare_seeds();
        if (!options.shared_resource_flow) pair_resources();
        build_model();
    }

    std::vector<const ds::MarketEvent*> ordered_purchases() const {
        std::vector<const ds::MarketEvent*> events;
        for (const auto& event : task_data.problem.market_plan) events.push_back(&event);
        std::sort(events.begin(), events.end(), [](const auto* a, const auto* b) {
            return std::tie(a->hour, a->order_index) < std::tie(b->hour, b->order_index);
        });
        return events;
    }

    void prepare_inputs() {
        const auto& problem = task_data.problem;
        std::array<Count, items> bought{}, last_supply{};
        for (const auto& event : problem.market_plan) {
            if (event.market_op != kag::M_BUY_PRODUCT && event.market_op != kag::M_BUY_ANIMAL) continue;
            if (event.hour <= hours - 3) bought[event.item] += event.quantity;
            else if (event.hour == hours - 2) last_supply[event.item] += event.quantity;
        }
        for (int item = 0; item < items; ++item) {
            const Count total = problem.shed_availability.back()[item];
            const Count last = total - problem.shed_availability[hours - 2][item];
            free_inputs[item] = std::max(Count(0), problem.start.shed[item] + bought[item] - total + std::min(last, last_supply[item]));
        }
        const auto purchases = ordered_purchases();
        for (int item : input_items) {
            std::vector<int> targets;
            bool produced = false;
            for (const auto& task : task_data.tasks) {
                if (task.input == item) targets.push_back(task.id);
                produced |= task.output == item && task.quantity > 0;
            }
            int first = hours;
            for (const auto* event : purchases)
                if ((event->market_op == kag::M_BUY_PRODUCT || event->market_op == kag::M_BUY_ANIMAL) && event->item == item && event->quantity > 0)
                    first = std::min(first, event->hour + 1);
            input_ready[item] = problem.start.shed[item] || produced ? 0 : first;
            if (produced) continue;
            std::vector<int> readiness(std::min(Count(targets.size()), problem.start.shed[item]), 0);
            for (const auto* event : purchases) {
                if ((event->market_op != kag::M_BUY_PRODUCT && event->market_op != kag::M_BUY_ANIMAL) || event->item != item || event->hour > hours - 3) continue;
                readiness.insert(readiness.end(), std::min(Count(event->quantity), Count(targets.size() - readiness.size())), event->hour + 1);
            }
            int index = int(targets.size()) - 1;
            for (int ready : readiness) if (ready > 0) late_inputs[targets[index--]] = ready;
        }
    }

    void prepare_seeds() {
        const auto& problem = task_data.problem;
        const auto purchases = ordered_purchases();
        for (int crop = 0; crop < crops; ++crop) {
            std::vector<int> targets;
            for (const auto& task : task_data.tasks) if (task.crop == crop) targets.push_back(task.id);
            if (targets.empty()) continue;
            std::vector<int> readiness(std::min(Count(targets.size()), problem.start.seeds[crop]), 0);
            for (const auto* event : purchases) {
                if (event->market_op != kag::M_BUY_SEED || event->item != crop) continue;
                readiness.insert(readiness.end(), std::min(Count(event->quantity), Count(targets.size() - readiness.size())), event->hour + 1);
            }
            require(readiness.size() == targets.size(), "not enough seeds");
            if (options.shared_seed_flow) {
                if (readiness.front() > 0) for (int task : targets) late_seeds[task] = readiness.front();
            } else {
                int index = int(targets.size()) - 1;
                for (auto ready = readiness.rbegin(); ready != readiness.rend(); ++ready)
                    if (*ready > 0) late_seeds[targets[index--]] = *ready;
            }
        }
        for (const auto& [task, ready] : late_seeds) task_data.early[task] = std::max(task_data.early[task], ready);
    }

    void pair_resources() {
        const auto& tasks = task_data.tasks;
        for (int item : input_items) {
            std::vector<int> targets, sources;
            for (const auto& task : tasks) {
                if (task.input == item) targets.push_back(task.id);
                if (task.output == item && task.quantity == 1 && !task_data.fixed_deadline.contains(task.id)) sources.push_back(task.id);
            }
            const Count deficit = std::max(Count(0), Count(targets.size()) - free_inputs[item]);
            if (!deficit || Count(sources.size()) < deficit) continue;
            sources.insert(sources.end(), std::min(free_inputs[item], Count(targets.size())), -1);
            std::vector<double> costs;
            for (int target : targets) for (int column = 0; column < int(sources.size()); ++column) {
                const int source = sources[column];
                const int base = source < 0 ? 1 + tail(tasks[target].point) : distance(tasks[source].point, tasks[target].point);
                const Count noise = (Count(options.seed) * 1009 + Count(target) * 9176 + Count(column) * 6113) % (Count(options.pair_jitter) + 1);
                costs.push_back(base * 100 + noise);
            }
            std::vector<int64_t> rows(targets.size()), columns(targets.size());
            require(solve_rectangular_linear_sum_assignment(targets.size(), sources.size(), costs.data(), false, rows.data(), columns.data()) == 0,
                    "resource assignment failed");
            for (int row = 0; row < int(targets.size()); ++row)
                if (sources[columns[row]] >= 0) resource_pairs.emplace_back(sources[columns[row]], targets[rows[row]]);
        }
    }

    void build_model() {
        const auto& tasks = task_data.tasks;
        const auto& early = task_data.early;
        const auto& late = task_data.late;
        const auto& outputs = task_data.fixed_deadline;
        std::set<int> paired, charged_outputs;
        for (const auto& [source, target] : resource_pairs) { paired.insert(source); paired.insert(target); }
        for (const auto& [task, ready] : late_inputs) paired.insert(task);
        for (const auto& [key, ids] : task_data.deliveries) if (!ids.empty()) charged_outputs.insert(ids.front());
        std::map<int, int> input_index;
        for (int index = 0; index < int(input_items.size()); ++index) input_index[input_items[index]] = index;
        const int output_dim = input_items.size();
        const int dimensions = output_dim + 1 + options.separate_delivery_routes;
        std::vector<int> selected, ordinary;
        for (const auto& task : tasks) {
            if (!options.include_all_tasks && task.input < 0 && !outputs.contains(task.id)) continue;
            selected.push_back(task.id);
            if (!outputs.contains(task.id) && !paired.contains(task.id)) ordinary.push_back(task.id);
        }
        for (int id : ordinary) {
            if (options.route_segments && !client_tasks.empty() && client_tasks.back().back() + 1 == id && tasks[client_tasks.back().back()].pattern == tasks[id].pattern)
                client_tasks.back().push_back(id);
            else client_tasks.push_back({id});
        }
        std::set<Point> unique_points(shed.begin(), shed.end());
        for (int task : selected) unique_points.insert(tasks[task].point);
        // Canonical location numbering; activity/depot/client order is unchanged.
        // The comparison remaps only physical location indices and their matrices.
        std::map<Point, int> location_index;
        for (Point point : unique_points) {
            location_index[point] = points.size();
            points.push_back(point);
            locations.emplace_back(point[0], point[1]);
        }
        const int end = points.size();
        points.push_back({-1000, -1000});
        locations.emplace_back(-1000, -1000);
        depots.emplace_back(end);
        const auto loc = [&](Point point) { return location_index.at(point); };
        constexpr Count inf = std::numeric_limits<Count>::max();
        auto add_vehicle = [&](int available, std::vector<pyvrp::Load> capacity, int start, int first, Count last, const std::string& name) {
            vehicles.emplace_back(available, capacity, start, 0, options.fixed_cost, first, last, options.route_hours,
                                  inf, 1, 0, 0, std::nullopt, std::vector<pyvrp::Load>{}, std::vector<size_t>{},
                                  std::numeric_limits<size_t>::max(), 0, 0, name);
        };
        std::vector<pyvrp::Load> capacity(dimensions);
        capacity[output_dim] = 200;
        if (options.separate_delivery_routes) capacity[output_dim + 1] = 1;
        if (options.worker_profile_fleet) {
            for (const auto& task : tasks) if (task.input >= 0) capacity[input_index.at(task.input)] += 1;
            std::array<int, 4> occupancy{1, 0, 0, 0};
            for (int worker = 0; worker < task_data.problem.worker_count; ++worker) {
                const int start = worker == 0 ? 0 : int(std::min_element(occupancy.begin(), occupancy.end()) - occupancy.begin());
                if (worker) ++occupancy[start];
                const int release = worker == 0 ? 0 : task_data.hires[worker - 1] + 1;
                depots.emplace_back(loc(shed[start]), 0, inf, 0, "worker_" + std::to_string(worker) + "_start");
                add_vehicle(1, capacity, worker + 1, release, hours, "worker_" + std::to_string(worker));
            }
        } else {
            depots.emplace_back(loc({4, 4}));
            add_vehicle(task_data.problem.worker_count, capacity, 1, 0, inf, "output");
            for (int item : input_items) {
                auto input_capacity = capacity;
                for (const auto& task : tasks) if (task.input == item) input_capacity[input_index.at(item)] += 1;
                depots.emplace_back(loc({4, 4}), 0, inf, 1);
                add_vehicle(task_data.problem.worker_count, input_capacity, depots.size() - 1, 0, inf, "input_" + std::to_string(item));
            }
        }
        for (const auto& group : client_tasks) {
            std::vector<pyvrp::Load> demand(dimensions);
            int first = -1000, last = hours;
            for (int offset = 0; offset < int(group.size()); ++offset) {
                const auto& task = tasks[group[offset]];
                if (task.input >= 0) demand[input_index.at(task.input)] += 1;
                first = std::max(first, early[task.id] - offset);
                last = std::min(last, late[task.id] - offset);
            }
            require(first <= last, "inconsistent segment window");
            clients.emplace_back(loc(tasks[group.front()].point), demand, std::vector<pyvrp::Load>{}, group.size(), first, last,
                                 0, 0, true, std::nullopt, "segment_" + std::to_string(group.front()));
        }
        for (const auto& [id, deadline] : outputs) {
            const auto& task = tasks[id];
            std::vector<pyvrp::Load> amount(dimensions);
            amount[output_dim] = task.quantity;
            if (options.separate_delivery_routes) amount[output_dim + 1] = 1;
            shipment_tasks.emplace_back(id, -1);
            shipments.emplace_back(loc(task.point), loc(nearest_shed(task.point)), early[id], late[id], 1, 0, deadline,
                                   int(options.separate_delivery_routes || charged_outputs.contains(id)), amount, 0, true, "output_" + std::to_string(id));
        }
        for (const auto& [source, target] : resource_pairs) {
            std::vector<pyvrp::Load> amount(dimensions);
            amount[input_index.at(tasks[target].input)] = 1;
            shipment_tasks.emplace_back(source, target);
            shipments.emplace_back(loc(tasks[source].point), loc(tasks[target].point), early[source], late[source], 1, early[target], late[target], 1,
                                   amount, 0, true, "resource_" + std::to_string(source) + "_" + std::to_string(target));
        }
        for (const auto& [target, ready] : late_inputs) {
            std::vector<pyvrp::Load> amount(dimensions);
            amount[input_index.at(tasks[target].input)] = 1;
            shipment_tasks.emplace_back(-1, target);
            const Point point = tasks[target].point;
            shipments.emplace_back(loc(nearest_shed(point)), loc(point), ready, late[target], 1, std::max(ready, early[target]), late[target], 1,
                                   amount, 0, true, "late_input_" + std::to_string(target));
        }
        distances.emplace_back(points.size(), points.size());
        durations.emplace_back(points.size(), points.size());
        for (int a = 0; a < int(points.size()); ++a) for (int b = 0; b < int(points.size()); ++b) {
            const int value = a == end || b == end ? 0 : distance(points[a], points[b]);
            distances[0](a, b) = value;
            durations[0](a, b) = value;
        }
    }

    pyvrp::ProblemData data() const {
        return {locations, clients, depots, vehicles, distances, durations, {}, shipments};
    }
};
}  // namespace day_constructor
