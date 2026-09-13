#include "event_routes.hpp"
#include "components/native_constructor_data/task_data.hpp"
#include "components/native_materialized_screen/data.hpp"
#include "components/native_materialized_screen/checkpoints.hpp"
#include "replay.hpp"
#include "ortools/sat/cp_model_checker.h"

namespace day_scheduler::event_detail {
namespace dc = day_constructor;
namespace ms = day_native::materialized_screen;
namespace ds = day_solver;
namespace sat = operations_research::sat;
using sat::IntVar;
using sat::BoolVar;
using sat::LinearExpr;
using operations_research::Domain;
using Count = ds::InventoryCount;
constexpr int hours = ds::HOURS, items = kag::N_ITEMS;

struct Node {
    enum Kind { task, pickup, deposit, start, end } kind;
    int value, worker;
    IntVar time, owner, x, y;
    BoolVar active, drop;
    std::vector<IntVar> cargo, quantity;
    std::vector<LinearExpr> change;
};

struct Model {
    const ds::DayProblem& problem;
    const EventRouteOptions& options;
    dc::TaskData data;
    sat::CpModelBuilder cp;
    ms::SpawnCheckpoints checkpoints;
    std::vector<Node> nodes;
    std::vector<int> starts, ends, stock_nodes, active_items;
    std::vector<Count> maximum;
    std::vector<std::vector<int>> bundles;
    std::vector<int> roots;
    int arcs = 0;

    IntVar number(Count lo, Count hi) { return cp.NewIntVar(Domain(lo, hi)); }
    BoolVar bit() { return cp.NewBoolVar(); }
    IntVar constant(Count value) { return cp.NewConstant(value); }
    LinearExpr distance(const Node& a, const Node& b) {
        if (a.kind == Node::task && b.kind == Node::task)
            return dc::distance(data.tasks[a.value].point, data.tasks[b.value].point);
        auto x = number(0, 9), y = number(0, 9);
        cp.AddAbsEquality(x, a.x - b.x); cp.AddAbsEquality(y, a.y - b.y);
        return x + y;
    }

    Model(const ds::DayProblem& p, const EventRouteOptions& o)
        : problem(p), options(o), data(p), checkpoints(cp, data.hires) {
        data.task_windows();
        bundles.resize(data.tasks.size()); roots.resize(data.tasks.size());
        for (const auto& task : data.tasks) {
            const bool early_return = task.predecessor >= 0 && data.fixed_deadline.contains(task.predecessor) &&
                data.fixed_deadline.at(task.predecessor) < hours - 1;
            const int root = o.bundle_tiles && task.predecessor >= 0 && !early_return ? roots[task.predecessor] : task.id;
            roots[task.id] = root; bundles[root].push_back(task.id);
        }
        std::array<Count, items> inputs{}, outputs{};
        for (const auto& task : data.tasks) {
            if (task.input >= 0) ++inputs[task.input];
            if (task.output >= 0) outputs[task.output] += task.quantity;
        }
        for (int item = 0; item < items; ++item) if (inputs[item] || outputs[item]) {
            active_items.push_back(item); maximum.push_back(inputs[item] + outputs[item]);
        }
        for (const auto& task : data.tasks) {
            Node n{Node::task, task.id, -1, number(data.early[task.id], data.late[task.id]),
                number(0, p.worker_count - 1), constant(task.point[0]), constant(task.point[1]), cp.TrueVar(), {}, {}, {}, {}};
            add(std::move(n));
        }
        for (const auto& group : bundles) for (int offset = 1; offset < int(group.size()); ++offset) {
            cp.AddEquality(nodes[group[offset]].owner, nodes[group.front()].owner);
            cp.AddEquality(nodes[group[offset]].time, nodes[group.front()].time + offset);
        }
        for (int worker = 0; worker < p.worker_count; ++worker) {
            const int release = checkpoints.releases[worker];
            starts.push_back(nodes.size());
            add({Node::start, -1, worker, constant(release), constant(worker),
                 checkpoints.x[worker].at(release), checkpoints.y[worker].at(release), cp.TrueVar(), {}, {}, {}, {}});
            ends.push_back(nodes.size());
            add({Node::end, -1, worker, constant(hours), constant(worker), constant(4), constant(4), cp.TrueVar(), {}, {}, {}, {}});
            for (int item = 0; item < items; ++item) if (inputs[item]) {
                int previous = -1;
                for (int slot = 0; slot < o.pickup_slots; ++slot) {
                    const int id = nodes.size();
                    add({Node::pickup, item, worker, number(0, hours - 1), constant(worker),
                         number(4, 5), number(4, 5), bit(), {}, {}, {}, {}});
                    stock_nodes.push_back(id);
                    if (previous >= 0) ordered_slots(previous, id);
                    previous = id;
                }
            }
            int previous = -1;
            for (int slot = 0; slot < o.deposit_slots; ++slot) {
                const int id = nodes.size();
                add({Node::deposit, -1, worker, number(0, hours - 1), constant(worker),
                     number(4, 5), number(4, 5), bit(), bit(), {}, {}, {}});
                stock_nodes.push_back(id);
                if (previous >= 0) ordered_slots(previous, id);
                previous = id;
            }
        }
        for (const auto& task : data.tasks) if (task.predecessor >= 0)
            cp.AddLessThan(event(task.predecessor), event(task.id));
        route_arcs();
        hire_positions();
        seed_stock();
        shed_stock();
    }

    LinearExpr event(int id) const { return nodes[id].time * (problem.worker_count + 1) + nodes[id].owner; }
    void ordered_slots(int a, int b) {
        cp.AddLessOrEqual(nodes[b].active, nodes[a].active);
        cp.AddLessThan(nodes[a].time, nodes[b].time).OnlyEnforceIf(nodes[b].active);
    }
    void add(Node n) {
        LinearExpr total;
        std::vector<BoolVar> placed;
        if (n.kind == Node::deposit) cp.AddLessOrEqual(n.drop, n.active);
        for (int index = 0; index < int(active_items.size()); ++index) {
            const int item = active_items[index];
            const auto cap = maximum[index];
            n.cargo.push_back(number(0, cap));
            IntVar quantity = constant(0);
            LinearExpr delta;
            if (n.kind == Node::task) {
                Count balance = 0, lowest = 0;
                for (int id : bundles[n.value]) {
                    const auto& task = data.tasks[id];
                    balance -= task.input == item;
                    lowest = std::min(lowest, balance);
                    if (task.output == item) balance += task.quantity;
                }
                delta = balance;
                cp.AddGreaterOrEqual(n.cargo.back(), balance - lowest);
            } else if (n.kind == Node::pickup && n.value == item) {
                quantity = number(0, cap);
                cp.AddLessOrEqual(quantity, n.active * cap);
                cp.AddGreaterOrEqual(quantity, n.active);
                delta = quantity;
            } else if (n.kind == Node::deposit) {
                quantity = number(0, cap);
                auto one = bit(); placed.push_back(one);
                cp.AddLessOrEqual(quantity, (LinearExpr(n.drop) + one) * cap);
                cp.AddGreaterOrEqual(quantity, one);
                cp.AddEquality(n.cargo.back(), 0).OnlyEnforceIf(n.drop);
                delta = -LinearExpr(quantity); total += quantity;
            }
            n.quantity.push_back(quantity); n.change.push_back(delta);
            if (n.kind == Node::start) cp.AddEquality(n.cargo.back(), 0);
        }
        if (n.kind == Node::deposit) {
            cp.AddEquality(LinearExpr::Sum(placed), LinearExpr(n.active) - n.drop);
            cp.AddGreaterOrEqual(total, n.active);
        }
        if (n.kind == Node::pickup || n.kind == Node::deposit) {
            cp.AddGreaterOrEqual(n.time, checkpoints.releases[n.worker]).OnlyEnforceIf(n.active);
            for (auto cargo : n.cargo) cp.AddEquality(cargo, 0).OnlyEnforceIf(n.active.Not());
        }
        nodes.push_back(std::move(n));
    }

    void route_arcs() {
        auto circuit = cp.AddCircuitConstraint();
        for (int worker = 0; worker < problem.worker_count; ++worker)
            circuit.AddArc(ends[worker], starts[(worker + 1) % problem.worker_count], cp.TrueVar());
        for (int id : stock_nodes) circuit.AddArc(id, id, nodes[id].active.Not());
        for (const auto& task : data.tasks) if (roots[task.id] != task.id)
            circuit.AddArc(task.id, task.id, cp.TrueVar());
        std::set<std::pair<int, int>> near;
        for (const auto& a : data.tasks) {
            if (roots[a.id] != a.id) continue;
            std::vector<std::pair<int, int>> ranked;
            for (const auto& b : data.tasks) if (a.id != b.id && roots[b.id] == b.id)
                ranked.emplace_back(dc::distance(a.point, b.point), b.id);
            std::sort(ranked.begin(), ranked.end());
            const int count = options.neighbors ? std::min(options.neighbors, int(ranked.size())) : int(ranked.size());
            for (int i = 0; i < count; ++i) near.emplace(a.id, ranked[i].second);
            for (const auto& b : data.tasks) if (a.pattern == b.pattern) near.emplace(a.id, b.id);
        }
        for (int a = 0; a < int(nodes.size()); ++a) for (int b = 0; b < int(nodes.size()); ++b) {
            const auto& from = nodes[a]; const auto& to = nodes[b];
            if (a == b || from.kind == Node::end || to.kind == Node::start) continue;
            if ((from.kind == Node::task && roots[a] != a) || (to.kind == Node::task && roots[b] != b)) continue;
            if (from.worker >= 0 && to.worker >= 0 && from.worker != to.worker) continue;
            if (from.kind == Node::task && to.kind == Node::task && !near.contains({a, b})) continue;
            if (from.kind == Node::task && to.kind == Node::task &&
                data.early[a] + 1 + dc::distance(data.tasks[a].point, data.tasks[b].point) > data.late[b]) continue;
            auto used = bit(); circuit.AddArc(a, b, used); ++arcs;
            cp.AddEquality(from.owner, to.owner).OnlyEnforceIf(used);
            const auto travel = to.kind == Node::end ? LinearExpr(0) : distance(from, to);
            const int duration = from.kind == Node::task ? int(bundles[a].size()) : from.kind != Node::start;
            cp.AddGreaterOrEqual(to.time, from.time + duration + travel).OnlyEnforceIf(used);
            for (int item = 0; item < int(active_items.size()); ++item)
                cp.AddEquality(to.cargo[item], from.cargo[item] + to.change[item]).OnlyEnforceIf(used);
        }
    }

    void hire_positions() {
        for (const auto& node : nodes) {
            if (node.kind == Node::start || node.kind == Node::end) continue;
            for (int hour : checkpoints.times) {
                auto x = number(0, 9), y = number(0, 9);
                auto alive = bit();
                std::vector<LinearExpr> xs, ys;
                std::vector<int64_t> active;
                for (int worker = 0; worker < problem.worker_count; ++worker) {
                    xs.push_back(checkpoints.x[worker].contains(hour) ? LinearExpr(checkpoints.x[worker].at(hour)) : LinearExpr(0));
                    ys.push_back(checkpoints.y[worker].contains(hour) ? LinearExpr(checkpoints.y[worker].at(hour)) : LinearExpr(0));
                    active.push_back(checkpoints.releases[worker] <= hour);
                }
                cp.AddElement(node.owner, xs, x); cp.AddElement(node.owner, ys, y); cp.AddElement(node.owner, active, alive);
                auto dx = number(0, 9), dy = number(0, 9);
                cp.AddAbsEquality(dx, node.x - x); cp.AddAbsEquality(dy, node.y - y);
                auto before = bit();
                cp.AddLessThan(node.time, hour).OnlyEnforceIf(before);
                cp.AddGreaterOrEqual(node.time, hour).OnlyEnforceIf(before.Not());
                cp.AddLessOrEqual(node.time + 1 + dx + dy, hour).OnlyEnforceIf({node.active, alive, before});
                cp.AddGreaterOrEqual(node.time, hour + dx + dy).OnlyEnforceIf({node.active, alive, before.Not()});
            }
        }
    }

    void seed_stock();
    void shed_stock();
    std::optional<std::array<kag::Action, hours>> materialize(const sat::CpSolverResponse&, std::string& error) const;
};

void Model::seed_stock() {
    for (int crop = 0; crop < kag::N_CROPS; ++crop) {
        std::vector<int> tasks;
        for (const auto& task : data.tasks) if (task.crop == crop) tasks.push_back(task.id);
        if (tasks.empty()) continue;
        std::set<int> times{hours - 1};
        for (const auto& order : problem.market_plan)
            if (order.market_op == kag::M_BUY_SEED && order.item == crop) times.insert(order.hour);
        for (int hour : times) {
            Count supply = problem.start.seeds[crop];
            for (const auto& order : problem.market_plan)
                if (order.market_op == kag::M_BUY_SEED && order.item == crop && order.hour < hour)
                    supply += order.quantity;
            LinearExpr planted;
            for (int id : tasks) {
                auto before = bit();
                cp.AddLessOrEqual(nodes[id].time, hour).OnlyEnforceIf(before);
                cp.AddGreaterThan(nodes[id].time, hour).OnlyEnforceIf(before.Not());
                planted += before;
            }
            cp.AddLessOrEqual(planted, supply);
        }
    }
}

void Model::shed_stock() {
    const int stride = problem.worker_count + 1;
    // Shared stock is observed in the engine's hour/worker order. Fixed sales
    // reserve stock after all workers; purchases enter stock after those sales.
    auto contribution = [&](int id, int item, LinearExpr checkpoint) -> LinearExpr {
        const auto& n = nodes[id];
        if (n.kind == Node::pickup && n.value != active_items[item]) return 0;
        auto before = bit();
        cp.AddLessThan(event(id), checkpoint).OnlyEnforceIf(before);
        cp.AddGreaterOrEqual(event(id), checkpoint).OnlyEnforceIf(before.Not());
        auto amount = number(0, maximum[item]);
        cp.AddEquality(amount, n.quantity[item]).OnlyEnforceIf(before);
        cp.AddEquality(amount, 0).OnlyEnforceIf(before.Not());
        return n.kind == Node::pickup ? -LinearExpr(amount) : LinearExpr(amount);
    };
    for (int index = 0; index < int(active_items.size()); ++index) {
        const int item = active_items[index];
        for (int id : stock_nodes) {
            const auto& target = nodes[id];
            if (target.kind != Node::pickup || target.value != item) continue;
            std::vector<int64_t> supply;
            for (int hour = 0; hour < hours; ++hour)
                supply.push_back(problem.start.shed[item] + data.bought(item, hour) -
                    (hour ? problem.shed_availability[hour - 1][item] : 0));
            auto stock = number(*std::min_element(supply.begin(), supply.end()), *std::max_element(supply.begin(), supply.end()));
            cp.AddElement(target.time, supply, stock);
            LinearExpr balance = stock - target.quantity[index];
            for (int other : stock_nodes) if (other != id) balance += contribution(other, index, event(id));
            cp.AddGreaterOrEqual(balance, 0).OnlyEnforceIf(target.active);
        }
        for (int hour = 0; hour < hours; ++hour) {
            if (hour != hours - 1 && problem.shed_availability[hour][item] ==
                (hour ? problem.shed_availability[hour - 1][item] : 0)) continue;
            LinearExpr balance = problem.start.shed[item] + data.bought(item, hour) - problem.shed_availability[hour][item];
            for (int id : stock_nodes) balance += contribution(id, index, hour * stride + problem.worker_count);
            cp.AddGreaterOrEqual(balance, 0);
        }
    }
}

std::optional<std::array<kag::Action, hours>> Model::materialize(
        const sat::CpSolverResponse& response, std::string& error) const {
    auto fail = [&](const std::string& reason) -> std::optional<std::array<kag::Action, hours>> { error = reason; return {}; };
    auto value = [&](IntVar variable) { return int(sat::SolutionIntegerValue(response, variable)); };
    const int workers = problem.worker_count;
    std::vector<std::array<int, hours + 1>> points(workers);
    std::vector<std::array<int, hours>> services(workers);
    for (auto& row : points) row.fill(-1);
    for (auto& row : services) row.fill(-1);
    auto fix = [&](int worker, int hour, int x, int y) {
        auto& previous = points[worker][hour];
        if (previous >= 0 && previous != x + 10 * y) return false;
        previous = x + 10 * y;
        return true;
    };
    for (int worker = 0; worker < workers; ++worker)
        for (const auto& [hour, x] : checkpoints.x[worker])
            if (!fix(worker, hour, value(x), value(checkpoints.y[worker].at(hour)))) return fail("inconsistent hire waypoint");
    for (int id = 0; id < int(nodes.size()); ++id) {
        const auto& node = nodes[id];
        if (node.kind == Node::start || node.kind == Node::end || !sat::SolutionBooleanValue(response, node.active)) continue;
        const int worker = value(node.owner), hour = value(node.time);
        if (hour < checkpoints.releases[worker] || services[worker][hour] >= 0) return fail("overlapping or premature service");
        services[worker][hour] = id;
        if (!fix(worker, hour, value(node.x), value(node.y)) || !fix(worker, hour + 1, value(node.x), value(node.y)))
            return fail("service conflicts with hire waypoint");
    }
    std::array<kag::Action, hours> schedule{};
    for (auto& turn : schedule) turn.n_units = 0;
    for (int hour = 0; hour < hours; ++hour)
        for (int worker = 0; worker < workers; ++worker) if (checkpoints.releases[worker] <= hour) {
            ++schedule[hour].n_units;
            schedule[hour].units[worker] = {kag::OP_PASS, 0, 1};
        }
    for (int worker = 0; worker < workers; ++worker) {
        int first = checkpoints.releases[worker];
        dc::require(points[worker][first] >= 0, "missing initial event waypoint");
        while (first < hours) {
            int last = first + 1;
            while (last <= hours && points[worker][last] < 0) ++last;
            if (last > hours) break;
            dc::Point position{points[worker][first] % 10, points[worker][first] / 10};
            const dc::Point target{points[worker][last] % 10, points[worker][last] / 10};
            if (dc::distance(position, target) > last - first) return fail("unreachable event waypoint");
            for (int hour = first; hour < last; ++hour) {
                auto& action = schedule[hour].units[worker];
                if (position[0] != target[0]) {
                    const int step = position[0] < target[0] ? 1 : -1;
                    position[0] += step; action.op = step > 0 ? kag::OP_EAST : kag::OP_WEST;
                } else if (position[1] != target[1]) {
                    const int step = position[1] < target[1] ? 1 : -1;
                    position[1] += step; action.op = step > 0 ? kag::OP_SOUTH : kag::OP_NORTH;
                }
            }
            first = last;
        }
    }
    for (int hour = 0; hour < hours; ++hour) {
        auto& turn = schedule[hour];
        for (int worker = 0; worker < turn.n_units; ++worker) {
            const int id = services[worker][hour];
            if (id < 0) continue;
            const auto& node = nodes[id];
            auto& action = turn.units[worker];
            if (node.kind == Node::task) {
                const auto& task = data.tasks[node.value];
                action = {static_cast<uint8_t>(task.op), static_cast<uint8_t>(std::max(0, task.arg)), static_cast<int32_t>(task.action_quantity)};
            } else if (node.kind == Node::deposit && sat::SolutionBooleanValue(response, node.drop)) action = {kag::OP_DROP, 0, 1};
            else {
                int selected = -1, quantity = 0;
                for (int item = 0; item < int(active_items.size()); ++item) if (value(node.quantity[item])) {
                    if (selected >= 0) return fail("selective transfer contains multiple items");
                    selected = active_items[item]; quantity = value(node.quantity[item]);
                }
                if (selected < 0 || quantity > ds::MAX_INPUT_COUNT) return fail("invalid transfer quantity");
                action = {static_cast<uint8_t>(node.kind == Node::pickup ? kag::OP_PICKUP : kag::OP_PLACE),
                          static_cast<uint8_t>(selected), quantity};
            }
        }
        for (const auto& order : problem.market_plan) if (order.hour == hour) {
            turn.n_orders = std::max(turn.n_orders, int(order.order_index) + 1);
            turn.orders[order.order_index] = {order.market_op, static_cast<uint8_t>(std::max(0, int(order.item))), order.quantity};
        }
        turn.finalize();
    }
    const auto replay = ds::replay_schedule(problem, schedule);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty()) {
        error = "strict replay rejected event routes";
        for (const auto& message : replay.errors) error += ": " + message;
        return {};
    }
    return schedule;
}
} // namespace day_scheduler::event_detail

namespace day_scheduler {
static Result solve_events(const day_solver::DayProblem& problem, const EventRouteOptions& options,
                           const day_native::InternalHint* witness) {
    namespace ed = event_detail;
    namespace sat = operations_research::sat;
    ed::dc::require(std::isfinite(options.seconds) && options.seconds >= 0 && options.neighbors >= 0 &&
        options.pickup_slots >= 1 && options.deposit_slots >= 1 && options.tuning >= 0 && options.tuning <= 2, "invalid event route options");
    const auto start = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };
    Result result;
    auto finish = [&] { result.seconds = elapsed(); return std::move(result); };
    if (!options.seconds) return finish();
    ed::Model model(problem, options);
    if (witness) {
        ed::dc::require(witness->assignments.size() == model.data.tasks.size(), "incomplete event witness");
        for (const auto& row : witness->assignments) {
            auto& node = model.nodes.at(row.task);
            model.cp.AddEquality(node.owner, row.worker);
            model.cp.AddEquality(node.time, row.hour.value());
        }
    }
    if (options.earliest_first) {
        ed::LinearExpr times;
        for (const auto& task : model.data.tasks) times += model.nodes[task.id].time;
        model.cp.Minimize(times);
    }
    result.stages.push_back({"events/build", "BUILT", elapsed()});
    result.stages.push_back({"events/variables", std::to_string(model.cp.Build().variables_size()), 0});
    result.stages.push_back({"events/arcs", std::to_string(model.arcs), 0});
    const auto error = sat::ValidateCpModel(model.cp.Build());
    ed::dc::require(error.empty(), "invalid event model: " + error);
    if (elapsed() >= options.seconds) return finish();
    sat::SatParameters parameters;
    parameters.set_max_time_in_seconds(options.seconds - elapsed()); parameters.set_num_search_workers(1);
    parameters.set_stop_after_first_solution(true);
    if (options.tuning) {
        parameters.set_cp_model_probing_level(0); parameters.set_symmetry_level(0);
    }
    if (options.tuning == 2) parameters.set_max_presolve_iterations(1);
    sat::Model solver;
    solver.Add(sat::NewSatParameters(parameters));
    const auto response = sat::SolveCpModel(model.cp.Build(), &solver);
    result.stages.push_back({"events/search", sat::CpSolverStatus_Name(response.status()), response.wall_time()});
    result.stages.push_back({"events/branches", std::to_string(response.num_branches()), 0});
    if (response.status() == sat::CpSolverStatus::FEASIBLE || response.status() == sat::CpSolverStatus::OPTIMAL) {
        std::string rejection;
        result.schedule = model.materialize(response, rejection);
        if (!result.schedule) result.stages.push_back({"events/replay", rejection, 0});
    }
    return finish();
}
Result event_routes(const day_solver::DayProblem& problem, const EventRouteOptions& options) {
    return solve_events(problem, options, nullptr);
}
Result event_task_witness(const day_solver::DayProblem& problem, const day_native::InternalHint& witness, const EventRouteOptions& options) {
    return solve_events(problem, options, &witness);
}
} // namespace day_scheduler
