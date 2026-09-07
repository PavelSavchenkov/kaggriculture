#pragma once
#include <memory>
#include "checkpoints.hpp"

namespace day_native::materialized_screen {

using Triple = std::tuple<int, int, int>;
struct Node {
    int kind, value, access;  // task, pickup, deposit
    IntVar time;
    Point point;
    BoolVar active;
    std::string repr() const {
        const std::string kind_name = kind == 0 ? "task" : kind == 1 ? "pickup" : "deposit";
        return "('" + kind_name + "', " + std::to_string(value) + (kind ? ", " + std::to_string(access) : "") + ")";
    }
};

struct ScreenModel {
    const ScreenData& data;
    const ScreenOptions& options;
    sat::CpModelBuilder model;
    std::unique_ptr<SpawnCheckpoints> checkpoints;
    std::vector<IntVar> task_time, concrete_worker;
    std::vector<std::vector<Node>> route_nodes;
    std::array<int, items> input_ready{};
    std::map<Key, BoolVar> route_worker, route_profile;
    std::map<Triple, BoolVar> deposit_active, delivery_uses;
    std::map<Triple, IntVar> deposit_time, delivered_units;
    std::vector<std::tuple<int, int, BoolVar>> cross_violations;
    std::vector<std::tuple<int, int, int, IntVar>> deficits;
    std::map<int, std::map<int, std::vector<int>>> pickup_requirements;

    explicit ScreenModel(const ScreenData& supplied) : data(supplied), options(data.options) {
        if (options.dynamic) checkpoints = std::make_unique<SpawnCheckpoints>(model, data.hires);
        for (const auto& task : data.tasks) task_time.push_back(integer(model, data.early[task.id], data.late[task.id], name("time", {task.id})));
        for (int item = 0; item < items; ++item) {
            bool produced = false;
            for (const auto& task : data.tasks) produced |= task.output == item && task.quantity > 0;
            if (produced || data.problem.start.shed[item] > 0) continue;
            input_ready[item] = hours;
            for (const auto& event : data.problem.market_plan)
                if ((event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL) &&
                    event.item == item && event.quantity > 0)
                    input_ready[item] = std::min(input_ready[item], int(event.hour) + 1);
        }
        shared_seeds();
        assign_workers();
        for (int route = 0; route < int(data.route_ids.size()); ++route) route_timing(route);
        deliveries();
        precedences();
    }

    void shared_seeds() {
        for (int crop = 0; crop < crops; ++crop) {
            std::vector<int> plants;
            for (const auto& task : data.tasks) if (task.crop == crop) plants.push_back(task.id);
            if (plants.empty()) continue;
            std::vector<std::pair<int, Count>> purchases;
            std::set<int> checkpoints{hours - 1};
            for (const auto& event : data.problem.market_plan) if (event.market_op == kag::M_BUY_SEED && event.item == crop) {
                purchases.push_back({event.hour, event.quantity}); checkpoints.insert(event.hour);
            }
            for (int hour : checkpoints) {
                Count available = data.problem.start.seeds[crop];
                for (const auto& [bought, quantity] : purchases) if (bought < hour) available += quantity;
                if (available >= Count(plants.size())) continue;
                if (!available) {
                    for (int task : plants) model.AddGreaterThan(task_time[task], hour);
                    continue;
                }
                LinearExpr used;
                for (int task : plants) {
                    const auto literal = boolean(model, name("seed_used", {task, hour}));
                    model.AddLessOrEqual(task_time[task], hour).OnlyEnforceIf(literal);
                    model.AddGreaterThan(task_time[task], hour).OnlyEnforceIf(literal.Not());
                    used += literal;
                }
                model.AddLessOrEqual(used, available);
            }
        }
    }

    void assign_workers() {
        for (int route = 0; route < int(data.route_ids.size()); ++route) {
            std::vector<BoolVar> choices;
            LinearExpr weighted;
            for (int worker = 0; worker < int(data.workers.size()); ++worker) {
                auto literal = boolean(model, name("route_worker", {route, worker}));
                route_worker[{route, worker}] = literal;
                choices.push_back(literal); weighted += literal * worker;
                model.AddHint(literal, worker == route);
                if (options.fixed_profile) {
                    const auto& allowed = data.source_groups[data.source_type.at(data.route_ids[route])];
                    if (std::find(allowed.begin(), allowed.end(), worker) == allowed.end()) model.AddEquality(literal, 0);
                }
            }
            model.AddExactlyOne(choices);
            auto worker = integer(model, 0, data.workers.size() - 1, name("concrete_worker", {route}));
            model.AddEquality(worker, weighted); concrete_worker.push_back(worker);
            for (int profile = 0; profile < int(data.profiles.size()); ++profile) {
                auto literal = boolean(model, name("route_profile", {route, profile}));
                route_profile[{route, profile}] = literal;
                LinearExpr members;
                for (int actual : data.groups[profile]) members += route_worker.at({route, actual});
                model.AddEquality(literal, members);
                model.AddHint(literal, profile == data.worker_group[route]);
            }
        }
        for (int worker = 0; worker < int(data.workers.size()); ++worker) {
            std::vector<BoolVar> choices;
            for (int route = 0; route < int(data.route_ids.size()); ++route) choices.push_back(route_worker.at({route, worker}));
            model.AddAtMostOne(choices);
        }
    }

    int start_bound(int profile, Point point) const {
        int best = 20;
        for (auto start : data.profiles[profile].starts) best = std::min(best, distance(start, point));
        return data.profiles[profile].release + best;
    }

    void route_timing(int route) {
        const auto& ids = data.route_tasks[route];
        auto sorted = data.hinted[route]; std::sort(sorted.begin(), sorted.end());
        std::vector<int> order;
        for (const auto& [hour, task] : sorted) order.push_back(task);
        const bool fixed = options.fixed_order && !options.free_order.contains(route);
        auto& pickups = pickup_requirements[route];
        if (!options.ignore_pickups) {
            std::array<Count, items> cargo{}, consumed{}, produced{};
            if (fixed) {
                for (int id : order) {
                    const auto& task = data.tasks[id];
                    if (task.input >= 0) {
                        if (cargo[task.input]) --cargo[task.input];
                        else pickups[task.input].push_back(id);
                    }
                    if (task.output >= 0) cargo[task.output] += task.quantity;
                }
            } else {
                for (int id : order) {
                    const auto& task = data.tasks[id];
                    if (task.input >= 0) ++consumed[task.input];
                    if (task.output >= 0) produced[task.output] += task.quantity;
                }
                for (int item = 0; item < items; ++item) if (consumed[item] > produced[item]) {
                    pickups[item] = {};
                    if (!produced[item]) for (int id : order) if (data.tasks[id].input == item) pickups[item].push_back(id);
                }
            }
        }
        std::vector<Node> nodes;
        std::map<int, int> task_node;
        std::vector<int> mandatory;
        for (int id : ids) {
            task_node[id] = nodes.size();
            nodes.push_back({0, id, -1, task_time[id], data.tasks[id].point, {}});
        }
        for (int task : order) mandatory.push_back(task_node.at(task));
        std::map<int, int> output_limits;
        for (int id : ids) if (data.tasks[id].output >= 0)
            for (const auto& [key, quantity] : data.requirements)
                if (key.first == data.tasks[id].output) {
                    if (!output_limits.contains(key.first)) output_limits[key.first] = key.second;
                    else output_limits[key.first] = std::max(output_limits[key.first], key.second);
                }
        if (options.fixed_availability) {
            std::map<int, int> fixed_limits;
            for (const auto& [task, deadline] : data.fixed_deadline) if (std::find(ids.begin(), ids.end(), task) != ids.end()) {
                const int item = data.tasks[task].output;
                fixed_limits[item] = std::max(deadline, output_limits.contains(item) ? output_limits[item] : -1);
            }
            output_limits = std::move(fixed_limits);
        }
        for (auto [worker, item] : data.surplus) if (worker == route) output_limits[item] = hours - 1;
        std::vector<int> deadlines;
        for (int deadline : data.deadlines)
            if (std::any_of(output_limits.begin(), output_limits.end(), [&](auto value) { return deadline <= value.second; })) deadlines.push_back(deadline);
        std::map<Key, int> pickup_node;
        for (const auto& [item, consumers] : pickups) {
            std::vector<BoolVar> alternatives;
            for (int access = 0; access < 4; ++access) {
                auto time = integer(model, 0, hours - 1, name("pickup_time", {route, item, access}));
                auto active = boolean(model, name("pickup_active", {route, item, access}));
                // Purchases execute after worker actions. With no initial stock
                // and no within-day producer, even the first pickup must wait.
                if (input_ready[item] > 0)
                    model.AddGreaterOrEqual(time, input_ready[item]).OnlyEnforceIf(active);
                for (int task : consumers) model.AddLessThan(time, task_time[task]).OnlyEnforceIf(active);
                alternatives.push_back(active); pickup_node[{item, access}] = nodes.size();
                nodes.push_back({1, item, access, time, shed[access], active});
            }
            model.AddExactlyOne(alternatives);
        }
        for (int deadline : deadlines) for (int access = 0; access < 4; ++access) {
            auto time = integer(model, 0, deadline, name("deposit_time", {route, deadline, access}));
            auto active = boolean(model, name("deposit_active", {route, deadline, access}));
            deposit_active[{route, deadline, access}] = active; deposit_time[{route, deadline, access}] = time;
            nodes.push_back({2, deadline, access, time, shed[access], active});
        }
        if (checkpoints) {
            RouteCheckpoints positions(*checkpoints, concrete_worker[route], name("route_checkpoint", {route}));
            for (const auto& node : nodes) positions.add_service(node.time, node.point, node.active);
        }
        auto hint_order = mandatory;
        for (const auto& [item, consumers] : pickups) {
            int position = 0;
            for (int i = 0; i < int(order.size()); ++i)
                if (std::find(consumers.begin(), consumers.end(), order[i]) != consumers.end() ||
                    (consumers.empty() && data.tasks[order[i]].input == item)) { position = i; break; }
            const auto target = data.tasks[order[position]].point;
            std::pair<int, int> best{100, 0};
            for (int access = 0; access < 4; ++access) {
                int travel = 20;
                for (auto start : data.workers[route].starts) travel = std::min(travel, distance(start, shed[access]));
                best = std::min(best, {travel + distance(shed[access], target), access});
            }
            const int node = pickup_node.at({item, best.second});
            model.AddHint(nodes[node].active, true); hint_order.insert(hint_order.begin() + position, node);
        }
        if (fixed) fixed_timing(route, nodes, mandatory, hint_order);
        else circuit_timing(route, nodes, hint_order);
        for (auto [hour, task] : data.hinted[route]) model.AddHint(task_time[task], std::clamp(hour, data.early[task], data.late[task]));
        route_nodes.push_back(std::move(nodes));
    }

    void fixed_timing(int route, const std::vector<Node>& nodes, const std::vector<int>& mandatory, const std::vector<int>& hints) {
        std::set<int> required(mandatory.begin(), mandatory.end());
        for (std::size_t i = 1; i < mandatory.size(); ++i) {
            const auto& a = nodes[mandatory[i - 1]]; const auto& b = nodes[mandatory[i]];
            model.AddGreaterOrEqual(b.time, a.time + distance(a.point, b.point) + 1);
        }
        for (int profile = 0; profile < int(data.profiles.size()); ++profile)
            for (const auto& node : nodes) {
                std::vector<BoolVar> condition{route_profile.at({route, profile})};
                if (node.active.index() >= 0) condition.push_back(node.active);
                model.AddGreaterOrEqual(node.time, start_bound(profile, node.point)).OnlyEnforceIf(condition);
            }
        // Fixed task order and the existing pickup-before-consumer constraints
        // already decide these directions. Avoid introducing redundant choices.
        std::map<int, int> task_rank, first_consumer;
        for (int rank = 0; rank < int(mandatory.size()); ++rank)
            task_rank[nodes[mandatory[rank]].value] = rank;
        for (const auto& [item, consumers] : pickup_requirements.at(route)) {
            int first = int(mandatory.size());
            for (int task : consumers) first = std::min(first, task_rank.at(task));
            first_consumer[item] = first;
        }
        auto pickup_before = [&](const Node& a, const Node& b) {
            return a.kind == 1 && b.kind == 0 && task_rank.at(b.value) >= first_consumer.at(a.value);
        };
        std::map<int, int> hinted;
        for (int i = 0; i < int(hints.size()); ++i) hinted[hints[i]] = i;
        for (int first = 0; first < int(nodes.size()); ++first)
            for (int second = first + 1; second < int(nodes.size()); ++second) {
                if (required.contains(first) && required.contains(second)) continue;
                const auto& a = nodes[first]; const auto& b = nodes[second];
                // Access alternatives for the same pickup are mutually exclusive.
                if (a.kind == 1 && b.kind == 1 && a.value == b.value) continue;
                std::vector<BoolVar> condition;
                if (a.active.index() >= 0) condition.push_back(a.active);
                if (b.active.index() >= 0) condition.push_back(b.active);
                const int travel = distance(a.point, b.point) + 1;
                if (pickup_before(a, b)) {
                    model.AddGreaterOrEqual(b.time, a.time + travel).OnlyEnforceIf(condition);
                    continue;
                }
                if (pickup_before(b, a)) {
                    model.AddGreaterOrEqual(a.time, b.time + travel).OnlyEnforceIf(condition);
                    continue;
                }
                const auto before = boolean(model, name(name("route", {route}) + "_before", {first, second}));
                condition.push_back(before);
                model.AddGreaterOrEqual(b.time, a.time + travel).OnlyEnforceIf(condition);
                condition.back() = before.Not();
                model.AddGreaterOrEqual(a.time, b.time + travel).OnlyEnforceIf(condition);
                if (hinted.contains(first) && hinted.contains(second)) model.AddHint(before, hinted[first] < hinted[second]);
            }
    }

    void circuit_timing(int route, const std::vector<Node>& nodes, const std::vector<int>& hints) {
        auto circuit = model.AddCircuitConstraint();
        std::map<Key, BoolVar> arcs;
        auto arc_name = [&](int a, int b) { return "arc[" + std::to_string(route) + "," + (a < 0 ? "start" : nodes[a].repr()) + "," + (b < 0 ? "end" : nodes[b].repr()) + "]"; };
        for (int i = 0; i < int(nodes.size()); ++i) {
            auto start = boolean(model, arc_name(-1, i)), end = boolean(model, arc_name(i, -1));
            arcs[{-1, i}] = start; arcs[{i, -1}] = end;
            circuit.AddArc(0, i + 1, start); circuit.AddArc(i + 1, 0, end);
            for (int profile = 0; profile < int(data.profiles.size()); ++profile)
                model.AddGreaterOrEqual(nodes[i].time, start_bound(profile, nodes[i].point)).OnlyEnforceIf({start, route_profile.at({route, profile})});
            if (nodes[i].active.index() >= 0) circuit.AddArc(i + 1, i + 1, nodes[i].active.Not());
        }
        for (int first = 0; first < int(nodes.size()); ++first)
            for (int second = 0; second < int(nodes.size()); ++second) if (first != second) {
                auto arc = boolean(model, arc_name(first, second)); arcs[{first, second}] = arc;
                circuit.AddArc(first + 1, second + 1, arc);
                model.AddGreaterOrEqual(nodes[second].time, nodes[first].time + distance(nodes[first].point, nodes[second].point) + 1).OnlyEnforceIf(arc);
            }
        int first = -1;
        for (int second : hints) { model.AddHint(arcs.at({first, second}), true); first = second; }
        if (first >= 0) model.AddHint(arcs.at({first, -1}), true);
    }

    void deliveries() {
        std::map<Triple, std::vector<BoolVar>> uses;
        for (const auto& task : data.tasks) {
            if (task.output < 0 || (options.fixed_availability && !data.fixed_deadline.contains(task.id) && !data.surplus.contains({data.owner[task.id], task.output}))) continue;
            int limit = -1;
            for (const auto& [key, quantity] : data.requirements) if (key.first == task.output) limit = std::max(limit, key.second);
            if (data.surplus.contains({data.owner[task.id], task.output})) limit = hours - 1;
            if (limit < 0) continue;
            LinearExpr choices;
            for (int deadline : data.deadlines) if (deadline <= limit)
                for (int access = 0; access < 4; ++access) {
                    const Triple deposit{data.owner[task.id], deadline, access};
                    if (!deposit_active.contains(deposit)) continue;
                    auto used = boolean(model, name("deposit_uses", {task.id, deadline, access}));
                    auto units = integer(model, 0, task.quantity, name("delivered_units", {task.id, deadline, access}));
                    model.AddLessOrEqual(units, used * task.quantity); model.AddGreaterOrEqual(units, used);
                    model.AddLessOrEqual(used, deposit_active.at(deposit));
                    model.AddLessThan(task_time[task.id], deposit_time.at(deposit)).OnlyEnforceIf(used);
                    delivered_units[{task.id, deadline, access}] = units; delivery_uses[{task.id, deadline, access}] = used;
                    uses[deposit].push_back(used); choices += units;
                }
            model.AddLessOrEqual(choices, task.quantity);
        }
        for (const auto& [key, active] : deposit_active) {
            LinearExpr required;
            for (auto used : uses[key]) required += used;
            model.AddLessOrEqual(active, required);
        }
        deposit_resets();
        std::map<Key, LinearExpr> returned;
        for (const auto& [key, units] : delivered_units) {
            const auto [task, deadline, access] = key;
            returned[{data.owner[task], data.tasks[task].output}] += units;
        }
        if (!options.ignore_availability)
            for (int item = 0; item < items; ++item) {
                LinearExpr stranded; Count maximum = 0;
                for (const auto& [key, quantity] : data.net) if (key.second == item && quantity > 0) {
                    auto leftover = integer(model, 0, quantity, name("unreturned", {key.first, item}));
                    model.AddGreaterOrEqual(leftover, quantity - returned[key]); stranded += leftover; maximum += quantity;
                }
                LinearExpr deficit;
                if (options.soft_availability && maximum) {
                    auto variable = integer(model, 0, maximum + std::max(Count(0), -data.problem.end_shed[item]), name("terminal_cargo_deficit", {item}));
                    deficits.push_back({item, hours - 1, -1, variable}); deficit = variable;
                }
                model.AddLessOrEqual(stranded, data.problem.end_shed[item] + deficit);
            }
        auto requirement = [&](int item, int deadline, int task, Count quantity) {
            LinearExpr delivered;
            for (const auto& [key, units] : delivered_units) {
                const auto [id, hour, access] = key;
                if (hour <= deadline && (task >= 0 ? id == task : data.tasks[id].output == item)) delivered += units;
            }
            if (options.soft_availability) {
                auto deficit = integer(model, 0, quantity, task >= 0 ? name("availability_deficit", {item, deadline, task}) : name("availability_deficit", {item, deadline}));
                deficits.push_back({item, deadline, task, deficit}); delivered += deficit;
            }
            if (task >= 0) model.AddEquality(delivered, quantity);
            else model.AddGreaterOrEqual(delivered, quantity);
        };
        if (options.fixed_availability) {
            for (const auto& [task, deadline] : data.fixed_deadline) requirement(data.tasks[task].output, deadline, task, data.tasks[task].quantity);
        } else for (const auto& [key, quantity] : data.requirements) requirement(key.first, key.second, -1, quantity);
        for (const auto& [key, units] : delivered_units) {
            const auto [task, deadline, access] = key;
            int preferred = 0;
            for (int point = 1; point < 4; ++point)
                if (distance(data.tasks[task].point, shed[point]) < distance(data.tasks[task].point, shed[preferred])) preferred = point;
            const bool selected = data.fixed_deadline.contains(task) && data.fixed_deadline.at(task) == deadline && access == preferred;
            model.AddHint(units, selected ? data.tasks[task].quantity : 0); model.AddHint(delivery_uses.at(key), selected);
        }
    }

    void deposit_resets() {
        std::map<Triple, std::map<int, std::vector<LinearExpr>>> grouped;
        for (const auto& [key, used] : delivery_uses) {
            const auto [task, deadline, access] = key;
            grouped[{data.owner[task], deadline, access}][data.tasks[task].output].push_back(used);
        }
        for (const auto& [key, by_item] : grouped) {
            if (by_item.size() < 2) continue;
            const auto [route, deadline, access] = key;
            std::vector<int> consumers;
            for (const auto& task : data.tasks) if (data.owner[task.id] == route && task.input >= 0) consumers.push_back(task.id);
            if (consumers.empty()) continue;
            LinearExpr count;
            for (const auto& [item, literals] : by_item) {
                auto used = boolean(model, name("deposited_item", {route, deadline, access, item}));
                model.AddMaxEquality(used, literals); count += used;
            }
            auto multiple = boolean(model, name("multi_item_drop", {route, deadline, access}));
            model.AddGreaterOrEqual(count, 2).OnlyEnforceIf(multiple); model.AddLessOrEqual(count, 1).OnlyEnforceIf(multiple.Not());
            for (int task : consumers) {
                const int gap = distance(shed[access], data.tasks[task].point) + 2;
                model.AddLinearConstraint(task_time[task] - deposit_time.at(key), Domain::FromIntervals({{-hours, 0}, {gap, hours}})).OnlyEnforceIf(multiple);
            }
        }
    }

    void precedences() {
        auto event = [&](int task) { return task_time[task] * data.workers.size() + concrete_worker[data.owner[task]]; };
        for (const auto& task : data.tasks) {
            const int previous = task.predecessor;
            if (previous < 0) continue;
            if (data.owner[previous] == data.owner[task.id]) model.AddLessThan(task_time[previous], task_time[task.id]);
            else if (options.soft_precedence) {
                auto violated = boolean(model, name("cross_precedence_violated", {previous, task.id}));
                model.AddLessThan(event(previous), event(task.id)).OnlyEnforceIf(violated.Not());
                model.AddGreaterOrEqual(event(previous), event(task.id)).OnlyEnforceIf(violated);
                cross_violations.push_back({previous, task.id, violated});
            } else if (!options.ignore_precedence) model.AddLessThan(event(previous), event(task.id));
        }
        if (options.soft_precedence || options.soft_availability) {
            LinearExpr penalty;
            for (const auto& [item, hour, task, deficit] : deficits) penalty += deficit * (cross_violations.size() + 1);
            for (const auto& [a, b, violated] : cross_violations) penalty += violated;
            model.Minimize(penalty);
            model.MutableProto()->mutable_objective()->set_scaling_factor(1);
        }
    }
};

} // namespace day_native::materialized_screen
