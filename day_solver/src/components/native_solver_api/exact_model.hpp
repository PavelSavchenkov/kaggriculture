#pragma once
#include "exact.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_checker.h"
#include "ortools/sat/cp_model_solver.h"
#include "problem_validation.hpp"
#include "replay.hpp"
#include "tile_graph.hpp"

#include "internal_hint.hpp"

namespace day_native::exact {
namespace ds = day_solver;
namespace sat = operations_research::sat;
using operations_research::Domain;
using sat::BoolVar;
using sat::IntVar;
using sat::LinearExpr;
using Count = ds::InventoryCount;
constexpr int hours = ds::HOURS;
constexpr int items = kag::N_ITEMS;
constexpr int crops = kag::N_CROPS;
constexpr std::array<std::array<int, 2>, 4> shed_points{{{4, 4}, {5, 4}, {4, 5}, {5, 5}}};

std::string name(const std::string& prefix, std::initializer_list<int> indices) {
    std::string result = prefix + "[";
    for (int index : indices) {
        if (result.back() != '[') result += ',';
        result += std::to_string(index);
    }
    return result + ']';
}

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

#include "hint_options.hpp"

struct Task {
    int tile, predecessor, input = -1, crop = -1;
    ds::TileWorkAction action;
};

struct TileLanguage {
    ds::TileGraph graph;
    std::vector<std::array<bool, hours>> task_hours;
};

TileLanguage trim(ds::TileGraph graph) {
    std::vector<std::vector<int>> forward(graph.nodes.size()), reverse(graph.nodes.size());
    for (const auto& arc : graph.arcs) {
        forward[arc.from].push_back(arc.to);
        reverse[arc.to].push_back(arc.from);
    }
    auto reachable = [&](std::vector<int> queue, const auto& adjacency) {
        std::vector<bool> seen(graph.nodes.size());
        for (int node : queue) seen[node] = true;
        for (std::size_t index = 0; index < queue.size(); ++index)
            for (int other : adjacency[queue[index]])
                if (!seen[other]) {
                    seen[other] = true;
                    queue.push_back(other);
                }
        return seen;
    };
    const auto from_start = reachable({0}, forward);
    const auto to_end = reachable(graph.terminals, reverse);
    std::erase_if(graph.arcs, [&](const auto& arc) {
        return !from_start[arc.from] || !to_end[arc.from] || !from_start[arc.to] || !to_end[arc.to];
    });
    std::erase_if(graph.terminals, [&](int node) { return !from_start[node] || !to_end[node]; });
    std::vector<std::array<bool, hours>> allowed(graph.task_count);
    for (const auto& arc : graph.arcs) {
        const auto& node = graph.nodes[arc.from];
        for (int task = node.prefix; task < arc.prefix; ++task) allowed[task][node.hour] = true;
    }
    return {std::move(graph), std::move(allowed)};
}

bool domains_imply_paths(const TileLanguage& language, const std::array<int, hours>& active) {
    const auto& graph = language.graph;
    const int count = graph.task_count;
    if (graph.terminals.empty()) return false;
    std::vector<std::vector<std::vector<int>>> following(hours, std::vector<std::vector<int>>(count + 1));
    std::vector<bool> suffix(count + 1);
    suffix[count] = true;
    for (int hour = hours - 1; hour >= 0; --hour) {
        std::vector<bool> preceding(count + 1);
        for (int before = 0; before <= count; ++before)
            for (int after = before; after <= std::min(count, before + active[hour]); ++after) {
                if (after > before && !language.task_hours[after - 1][hour]) break;
                if (suffix[after]) {
                    following[hour][before].push_back(after);
                    preceding[before] = true;
                }
            }
        suffix = std::move(preceding);
    }
    if (!suffix[0]) return false;
    std::map<std::pair<int, int>, int> transitions;
    for (const auto& arc : graph.arcs) {
        const auto key = std::pair(arc.from, arc.prefix);
        require(!transitions.contains(key) || transitions[key] == arc.to, "tile language is not deterministic");
        transitions[key] = arc.to;
    }
    std::set<int> frontier{0};
    for (int hour = 0; hour < hours; ++hour) {
        std::set<int> reached;
        for (int source : frontier)
            for (int after : following[hour][graph.nodes[source].prefix]) {
                const auto found = transitions.find({source, after});
                if (found == transitions.end()) return false;
                reached.insert(found->second);
            }
        frontier = std::move(reached);
    }
    return !frontier.empty() && std::all_of(frontier.begin(), frontier.end(), [&](int node) {
        return std::find(graph.terminals.begin(), graph.terminals.end(), node) != graph.terminals.end();
    });
}

struct ExactModel {
    const ds::DayProblem& problem;
    int workers;
    int pruned_task_variables = 0;
    sat::CpModelBuilder model;
    std::vector<Task> tasks;
    std::vector<int> releases;
    std::vector<std::array<IntVar, hours + 1>> x, y;
    std::vector<IntVar> events, cargo, pickup_quantity, place_quantity;
    std::vector<BoolVar> task_at, pickup, place, drop;
    std::vector<std::array<std::vector<BoolVar>, hours>> worker_tasks;
    std::vector<TileLanguage> languages;
    std::array<Count, items> total_input{}, total_output{}, max_stock{}, max_cargo{};

    int task_index(int task, int worker, int hour) const { return (task * workers + worker) * hours + hour; }
    int inventory_index(int worker, int hour, int item) const { return (worker * hours + hour) * items + item; }
    int cargo_index(int worker, int hour, int item) const { return (worker * (hours + 1) + hour) * items + item; }
    static bool exists(BoolVar literal) { return literal.index() >= 0; }
    static bool exists(IntVar variable) { return variable.index() >= 0; }
    static LinearExpr quantity(IntVar variable) { return exists(variable) ? LinearExpr(variable) : LinearExpr(0); }

    explicit ExactModel(const ds::DayProblem& supplied, const HintOptions& hints) : problem(supplied), workers(problem.worker_count),
        releases{0}, x(workers), y(workers), worker_tasks(workers) {
        require(problem.format_version == 3 && ds::validate_problem(problem).empty(), "valid v3 input required");
        auto purchases = problem.market_plan;
        std::sort(purchases.begin(), purchases.end(), [](const auto& a, const auto& b) {
            return std::pair(a.hour, a.order_index) < std::pair(b.hour, b.order_index);
        });
        for (const auto& event : purchases)
            if (event.market_op == kag::M_HIRE) releases.push_back(event.hour + 1);
        require(int(releases.size()) == workers, "fixed hires do not match worker count");
        for (const auto& work : problem.tile_work) {
            int predecessor = -1;
            for (const auto& action : work.actions) {
                const int input = action.op == kag::OP_FEED ? kag::WHEAT :
                    action.op == kag::OP_FERTILIZE ? kag::FERTILIZER : action.op == kag::OP_PLACE ? action.arg : -1;
                tasks.push_back({work.tile, predecessor, input, action.op == kag::OP_PLANT ? action.arg : -1, action});
                predecessor = tasks.size() - 1;
                if (input >= 0) ++total_input[input];
                if (action.output_item >= 0) total_output[action.output_item] += action.output_quantity;
            }
        }
        for (auto graph : ds::build_tile_graphs(problem)) languages.push_back(trim(std::move(graph)));
        for (int item = 0; item < items; ++item) {
            max_stock[item] = problem.start.shed[item] + total_output[item];
            max_cargo[item] = total_input[item] + total_output[item];
        }
        for (const auto& event : purchases)
            if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)
                max_stock[event.item] += event.quantity;
        if (bounded_storage())
            for (int item = 0; item < items; ++item)
                max_cargo[item] = std::max(max_cargo[item], max_stock[item]);
        for (int worker = 0; worker < workers; ++worker)
            for (int hour = 0; hour <= hours; ++hour) x[worker][hour] = integer(0, 9, name("x", {worker, hour}));
        for (int worker = 0; worker < workers; ++worker)
            for (int hour = 0; hour <= hours; ++hour) y[worker][hour] = integer(0, 9, name("y", {worker, hour}));
        add_spawns();
        add_tasks(hints.task_domains(tasks.size(), workers));
        add_inventory_actions();
        add_shed();
        add_seeds();
        add_tile_paths();
    }

    IntVar integer(Count lower, Count upper, const std::string& label) {
        return model.NewIntVar(Domain(lower, upper)).WithName(label);
    }
    BoolVar boolean(const std::string& label) { return model.NewBoolVar().WithName(label); }

    void add_spawns() {
        model.AddEquality(x[0][0], shed_points[0][0]);
        model.AddEquality(y[0][0], shed_points[0][1]);
        int active = 1;
        for (int hour = 0; hour < hours; ++hour) {
            const int count = std::count(releases.begin() + 1, releases.end(), hour + 1);
            if (!count) continue;
            std::array<LinearExpr, 4> occupancy;
            for (int point = 0; point < 4; ++point)
                for (int existing = 0; existing < active; ++existing) {
                    const auto literal = boolean(name("hire_occ", {hour, existing, point}));
                    auto table = model.AddAllowedAssignments({x[existing][hour + 1], y[existing][hour + 1], literal});
                    for (int px = 0; px < 10; ++px)
                        for (int py = 0; py < 10; ++py)
                            table.AddTuple({px, py, int(shed_points[point] == std::array<int, 2>{px, py})});
                    occupancy[point] += literal;
                }
            for (int index = 0; index < count; ++index) {
                const int worker = active++;
                std::vector<BoolVar> choices;
                for (int point = 0; point < 4; ++point) choices.push_back(boolean(name("spawn", {worker, point})));
                model.AddExactlyOne(choices);
                for (int point = 0; point < 4; ++point) {
                    for (int other = 0; other < 4; ++other) {
                        if (other < point) model.AddLessThan(occupancy[point], occupancy[other]).OnlyEnforceIf(choices[point]);
                        if (other > point) model.AddLessOrEqual(occupancy[point], occupancy[other]).OnlyEnforceIf(choices[point]);
                    }
                    model.AddEquality(x[worker][hour + 1], shed_points[point][0]).OnlyEnforceIf(choices[point]);
                    model.AddEquality(y[worker][hour + 1], shed_points[point][1]).OnlyEnforceIf(choices[point]);
                }
                for (int point = 0; point < 4; ++point) occupancy[point] += choices[point];
                require(releases[worker] == hour + 1, "hire release order is inconsistent");
            }
        }
        require(active == workers, "not every worker has a spawn event");
    }

    void add_tasks(const std::vector<unsigned char>& permitted) {
        task_at.resize(tasks.size() * workers * hours);
        std::vector<int> prefix(problem.start.managed_tiles.size());
        for (int task = 0; task < int(tasks.size()); ++task) {
            const auto& value = tasks[task];
            const auto& tile = problem.start.managed_tiles[value.tile];
            const auto& allowed = languages[value.tile].task_hours[prefix[value.tile]++];
            std::vector<BoolVar> choices;
            LinearExpr event_sum;
            for (int worker = 0; worker < workers; ++worker)
                for (int hour = releases[worker]; hour < hours; ++hour) {
                    if (!allowed[hour]) continue;
                    if (!permitted.empty() && !permitted[task_index(task, worker, hour)]) {
                        ++pruned_task_variables;
                        continue;
                    }
                    const auto literal = boolean(name("task", {task, worker, hour}));
                    task_at[task_index(task, worker, hour)] = literal;
                    worker_tasks[worker][hour].push_back(literal);
                    choices.push_back(literal);
                    model.AddEquality(x[worker][hour], tile.x).OnlyEnforceIf(literal);
                    model.AddEquality(y[worker][hour], tile.y).OnlyEnforceIf(literal);
                    event_sum += literal * (hour * workers + worker);
                }
            model.AddExactlyOne(choices);
            const auto event = integer(0, hours * workers - 1, name("task_event", {task}));
            model.AddEquality(event, event_sum);
            events.push_back(event);
            if (value.predecessor >= 0) model.AddLessThan(events[value.predecessor], event);
        }
    }

    void at_shed(int worker, int hour, BoolVar literal) {
        auto table = model.AddAllowedAssignments({x[worker][hour], y[worker][hour]});
        for (const auto& point : shed_points) table.AddTuple({point[0], point[1]});
        table.OnlyEnforceIf(literal);
    }

    void add_inventory_actions() {
        const int width = workers * hours * items;
        pickup.resize(width); place.resize(width);
        pickup_quantity.resize(width); place_quantity.resize(width);
        drop.resize(workers * hours);
        cargo.resize(workers * (hours + 1) * items);
        for (int worker = 0; worker < workers; ++worker)
            for (int hour = 0; hour <= hours; ++hour)
                for (int item = 0; item < items; ++item)
                    cargo[cargo_index(worker, hour, item)] = integer(0, max_cargo[item], name("cargo", {worker, hour, item}));
        for (int worker = 0; worker < workers; ++worker)
            for (int item = 0; item < items; ++item) model.AddEquality(cargo[cargo_index(worker, releases[worker], item)], 0);
        for (int worker = 0; worker < workers; ++worker)
            for (int hour = releases[worker]; hour < hours; ++hour) {
                std::vector<BoolVar> inventory;
                for (int item = 0; item < items; ++item) {
                    const Count limit = bounded_storage() ? max_stock[item] : total_input[item];
                    if (!limit) continue;
                    const int key = inventory_index(worker, hour, item);
                    const auto literal = boolean(name("pickup", {worker, hour, item}));
                    const auto amount = integer(0, std::min(limit, ds::MAX_INPUT_COUNT), name("pickup_quantity", {worker, hour, item}));
                    model.AddGreaterOrEqual(amount, literal);
                    model.AddLessOrEqual(amount, literal * limit);
                    at_shed(worker, hour, literal);
                    pickup[key] = literal; pickup_quantity[key] = amount;
                    inventory.push_back(literal);
                }
                for (int item = 0; item < items; ++item) {
                    if (!(bounded_storage() ? max_cargo[item] : total_output[item])) continue;
                    const int key = inventory_index(worker, hour, item);
                    const auto literal = boolean(name("place", {worker, hour, item}));
                    // UnitAction.n is signed32; larger cargo can be returned with DROP.
                    const auto amount = integer(0, std::min(max_cargo[item], ds::MAX_INPUT_COUNT), name("place_quantity", {worker, hour, item}));
                    model.AddGreaterOrEqual(amount, literal);
                    model.AddLessOrEqual(amount, literal * max_cargo[item]);
                    at_shed(worker, hour, literal);
                    place[key] = literal; place_quantity[key] = amount;
                    inventory.push_back(literal);
                }
                const auto dropped = boolean(name("drop", {worker, hour}));
                LinearExpr carried;
                for (int item = 0; item < items; ++item) carried += cargo[cargo_index(worker, hour, item)];
                model.AddGreaterOrEqual(carried, dropped);
                at_shed(worker, hour, dropped);
                drop[worker * hours + hour] = dropped;
                inventory.push_back(dropped);
                const auto busy = boolean(name("busy", {worker, hour}));
                model.AddEquality(busy, LinearExpr::Sum(worker_tasks[worker][hour]) + LinearExpr::Sum(inventory));
                const auto dx = integer(0, 9, name("dx", {worker, hour}));
                const auto dy = integer(0, 9, name("dy", {worker, hour}));
                model.AddAbsEquality(dx, x[worker][hour + 1] - x[worker][hour]);
                model.AddAbsEquality(dy, y[worker][hour + 1] - y[worker][hour]);
                model.AddLessOrEqual(dx + dy, 1);
                model.AddEquality(dx + dy, 0).OnlyEnforceIf(busy);
                for (int item = 0; item < items; ++item) {
                    const int key = inventory_index(worker, hour, item);
                    LinearExpr change = quantity(pickup_quantity[key]) - quantity(place_quantity[key]);
                    for (int task = 0; task < int(tasks.size()); ++task) {
                        const auto literal = task_at[task_index(task, worker, hour)];
                        if (!exists(literal)) continue;
                        if (tasks[task].input == item) change -= literal;
                        if (tasks[task].action.output_item == item) change += literal * tasks[task].action.output_quantity;
                    }
                    const auto before = cargo[cargo_index(worker, hour, item)];
                    const auto after = cargo[cargo_index(worker, hour + 1, item)];
                    model.AddEquality(after, 0).OnlyEnforceIf(dropped);
                    model.AddEquality(after, before + change).OnlyEnforceIf(dropped.Not());
                }
            }
    }

    bool bounded_storage() const {
        return problem.start.shed_capacity != std::numeric_limits<int16_t>::max();
    }

#include "bounded_shed.hpp"

    void add_shed() {
        if (bounded_storage()) { add_bounded_shed(); return; }
        std::array<LinearExpr, items> shed;
        for (int item = 0; item < items; ++item) shed[item] = problem.start.shed[item];
        for (int hour = 0; hour < hours; ++hour) {
            for (int worker = 0; worker < workers; ++worker) {
                if (releases[worker] > hour) continue;
                for (int item = 0; item < items; ++item) {
                    const int key = inventory_index(worker, hour, item);
                    const auto deposited = integer(0, max_cargo[item], name("drop_quantity", {worker, hour, item}));
                    model.AddMultiplicationEquality(deposited, {cargo[cargo_index(worker, hour, item)], drop[worker * hours + hour]});
                    const auto value = integer(0, max_stock[item], name("shed_stage", {hour, worker, item}));
                    model.AddEquality(value, shed[item] - quantity(pickup_quantity[key]) + quantity(place_quantity[key]) + deposited);
                    shed[item] = value;
                }
            }
            for (int item = 0; item < items; ++item) {
                const Count removed = problem.shed_availability[hour][item] - (hour ? problem.shed_availability[hour - 1][item] : 0);
                model.AddGreaterOrEqual(shed[item], removed);
                Count purchased = 0;
                for (const auto& event : problem.market_plan)
                    if (event.hour == hour && event.item == item &&
                        (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)) purchased += event.quantity;
                const auto value = integer(0, max_stock[item], name("shed", {hour + 1, item}));
                model.AddEquality(value, shed[item] - removed + purchased);
                shed[item] = value;
            }
        }
        for (int item = 0; item < items; ++item) {
            for (int worker = 0; worker < workers; ++worker) shed[item] += cargo[cargo_index(worker, hours, item)];
            model.AddEquality(shed[item], problem.end_shed[item]);
        }
    }

    void add_seeds() {
        for (int crop = 0; crop < crops; ++crop) {
            int demand = 0;
            for (const auto& task : tasks) demand += task.crop == crop;
            Count total = problem.start.seeds[crop];
            for (const auto& event : problem.market_plan)
                if (event.market_op == kag::M_BUY_SEED && event.item == crop) total += event.quantity;
            require(total - demand == problem.end_seeds[crop], "deterministic end seed mismatch");
            LinearExpr used;
            Count available = problem.start.seeds[crop];
            for (int hour = 0; hour < hours; ++hour) {
                for (int task = 0; task < int(tasks.size()); ++task)
                    if (tasks[task].crop == crop)
                        for (int worker = 0; worker < workers; ++worker) {
                            const auto literal = task_at[task_index(task, worker, hour)];
                            if (exists(literal)) used += literal;
                        }
                model.AddLessOrEqual(used, available);
                for (const auto& event : problem.market_plan)
                    if (event.market_op == kag::M_BUY_SEED && event.item == crop && event.hour == hour) available += event.quantity;
            }
        }
    }

    void add_tile_paths() {
        std::array<int, hours> active{};
        for (int hour = 0; hour < hours; ++hour)
            for (int release : releases) active[hour] += release <= hour;
        for (const auto& language : languages) {
            const auto& graph = language.graph;
            if (graph.terminals.empty()) {
                model.AddBoolOr({});
                continue;
            }
            if (domains_imply_paths(language, active)) continue;
            std::vector<IntVar> prefixes;
            LinearExpr previous;
            for (int hour = 0; hour < hours; ++hour) {
                LinearExpr count;
                for (int task = 0; task < int(tasks.size()); ++task)
                    if (tasks[task].tile == graph.tile)
                        for (int worker = 0; worker < workers; ++worker) {
                            const auto literal = task_at[task_index(task, worker, hour)];
                            if (exists(literal)) count += literal;
                        }
                const auto prefix = integer(0, graph.task_count, name("tile_prefix", {graph.tile, hour}));
                model.AddEquality(prefix, previous + count);
                prefixes.push_back(prefix);
                previous = prefix;
            }
            auto automaton = model.AddAutomaton(prefixes, 0, graph.terminals);
            for (const auto& arc : graph.arcs) automaton.AddTransition(arc.from, arc.to, arc.prefix);
        }
    }

    std::array<kag::Action, hours> materialize(const sat::CpSolverResponse& response) const {
        std::array<kag::Action, hours> result{};
        for (int hour = 0; hour < hours; ++hour) {
            auto& turn = result[hour];
            turn.n_units = 0;
            for (int worker = 0; worker < workers; ++worker) {
                if (releases[worker] > hour) break;
                auto& action = turn.units[turn.n_units++];
                bool assigned = false;
                for (int task = 0; task < int(tasks.size()); ++task) {
                    const auto literal = task_at[task_index(task, worker, hour)];
                    if (exists(literal) && sat::SolutionBooleanValue(response, literal)) {
                        require(!assigned, "multiple simultaneous tasks");
                        action = {tasks[task].action.op, static_cast<uint8_t>(std::max(0, int(tasks[task].action.arg))), 1};
                        assigned = true;
                    }
                }
                for (int item = 0; item < items; ++item) {
                    const int key = inventory_index(worker, hour, item);
                    for (bool take : {true, false}) {
                        const auto literal = take ? pickup[key] : place[key];
                        if (exists(literal) && sat::SolutionBooleanValue(response, literal)) {
                            require(!assigned, "simultaneous inventory and farm actions");
                            const auto count = sat::SolutionIntegerValue(response, take ? pickup_quantity[key] : place_quantity[key]);
                            require(count > 0 && count <= ds::MAX_INPUT_COUNT, "inventory command quantity out of range");
                            action = {take ? kag::OP_PICKUP : kag::OP_PLACE, static_cast<uint8_t>(item), static_cast<int32_t>(count)};
                            assigned = true;
                        }
                    }
                }
                if (sat::SolutionBooleanValue(response, drop[worker * hours + hour])) {
                    require(!assigned, "simultaneous DROP and other action");
                    action = {kag::OP_DROP, 0, 1};
                    assigned = true;
                }
                if (!assigned) {
                    const int dx = sat::SolutionIntegerValue(response, x[worker][hour + 1]) - sat::SolutionIntegerValue(response, x[worker][hour]);
                    const int dy = sat::SolutionIntegerValue(response, y[worker][hour + 1]) - sat::SolutionIntegerValue(response, y[worker][hour]);
                    require(std::abs(dx) + std::abs(dy) <= 1, "illegal movement");
                    action.op = dx == 1 ? kag::OP_EAST : dx == -1 ? kag::OP_WEST :
                        dy == 1 ? kag::OP_SOUTH : dy == -1 ? kag::OP_NORTH : kag::OP_PASS;
                }
            }
            for (const auto& event : problem.market_plan) {
                if (event.hour != hour) continue;
                turn.n_orders = std::max(turn.n_orders, int(event.order_index) + 1);
                turn.orders[event.order_index] = {event.market_op, static_cast<uint8_t>(std::max(0, int(event.item))), event.quantity};
            }
            turn.finalize();
        }
        return result;
    }
};


#include "exact_hints.hpp"
} // namespace day_native::exact
