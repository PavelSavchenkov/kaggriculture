#pragma once
#include "replay.hpp"
#include "../../storage.hpp"
#include <limits>

namespace day_native::materialized_screen {

// This is a candidate constructor. The coarse model is a relaxation; only a
// successful full replay makes the constructed schedule acceptable.
std::optional<std::array<kag::Action, hours>> materialize(
        const ScreenModel& screen, const sat::CpSolverResponse& response, std::string& rejection, double repair_seconds,
        std::array<kag::Action, hours>* rejected_schedule = nullptr) {
    const auto& data = screen.data;
    const auto& problem = data.problem;
    auto fail = [&](const std::string& reason) -> std::optional<std::array<kag::Action, hours>> {
        rejection = reason;
        return {};
    };
    if (!screen.checkpoints || !data.options.fixed_order || !data.options.free_order.empty())
        return fail("requires fixed route order and hire checkpoints");
    const auto& checkpoints = *screen.checkpoints;
    const int workers = problem.worker_count;
    struct Service { int kind = -1, value = -1; };
    std::vector<std::array<Service, hours>> services(workers);
    std::map<Key, Count> selected_pickups;
    std::vector<std::array<int, hours + 1>> points(workers);
    for (auto& timeline : points) timeline.fill(-1);
    auto fix = [&](int worker, int hour, Point point) {
        const int key = point[0] + 10 * point[1];
        auto& previous = points[worker][hour];
        if (previous >= 0 && previous != key) return false;
        previous = key;
        return true;
    };
    auto value = [&](IntVar variable) { return int(sat::SolutionIntegerValue(response, variable)); };
    for (int worker = 0; worker < workers; ++worker)
        for (const auto& [hour, x] : checkpoints.x[worker])
            if (!fix(worker, hour, {value(x), value(checkpoints.y[worker].at(hour))}))
                return fail("inconsistent hire waypoint");
    for (int route = 0; route < int(screen.route_nodes.size()); ++route) {
        const int worker = value(screen.concrete_worker[route]);
        for (const auto& node : screen.route_nodes[route]) {
            if (node.active.index() >= 0 && !sat::SolutionBooleanValue(response, node.active)) continue;
            const int hour = value(node.time);
            if (hour < checkpoints.releases[worker] || services[worker][hour].kind >= 0)
                return fail("overlapping or premature service");
            services[worker][hour] = {node.kind, node.value};
            if (node.kind == 1 && screen.extra_pickups)
                selected_pickups[{worker, hour}] = value(screen.pickup_units.at({route, node.value}));
            if (!fix(worker, hour, node.point) || !fix(worker, hour + 1, node.point))
                return fail("service conflicts with hire waypoint");
        }
    }
    std::array<kag::Action, hours> schedule{};
    for (auto& turn : schedule) turn.n_units = 0;
    for (int hour = 0; hour < hours; ++hour)
        for (int worker = 0; worker < workers; ++worker)
            if (checkpoints.releases[worker] <= hour) {
                ++schedule[hour].n_units;
                schedule[hour].units[worker] = {kag::OP_PASS, 0, 1};
            }
    for (int worker = 0; worker < workers; ++worker) {
        int first = checkpoints.releases[worker];
        require(points[worker][first] >= 0, "missing initial waypoint");
        while (first < hours) {
            int last = first + 1;
            while (last <= hours && points[worker][last] < 0) ++last;
            if (last > hours) break;
            Point position{points[worker][first] % 10, points[worker][first] / 10};
            const Point target{points[worker][last] % 10, points[worker][last] / 10};
            if (distance(position, target) > last - first) return fail("unreachable waypoint");
            for (int hour = first; hour < last; ++hour) {
                auto& action = schedule[hour].units[worker];
                if (position[0] != target[0]) {
                    const int step = position[0] < target[0] ? 1 : -1;
                    position[0] += step;
                    action.op = step > 0 ? kag::OP_EAST : kag::OP_WEST;
                } else if (position[1] != target[1]) {
                    const int step = position[1] < target[1] ? 1 : -1;
                    position[1] += step;
                    action.op = step > 0 ? kag::OP_SOUTH : kag::OP_NORTH;
                }
            }
            first = last;
        }
    }
    using Inventory = std::array<Count, items>;
    std::vector<std::array<Inventory, hours>> planned_returns(workers);
    for (const auto& [key, units] : screen.delivered_units) {
        const auto [task, deadline, access] = key;
        const int route = data.owner[task], worker = value(screen.concrete_worker[route]);
        const int hour = value(screen.deposit_time.at({route, deadline, access}));
        planned_returns[worker][hour][data.tasks[task].output] += sat::SolutionIntegerValue(response, units);
    }
    // Minimum cargo needed before the next opportunity to pick up this item.
    // Deposits can return only surplus above this amount.
    std::vector<std::array<Inventory, hours + 1>> need(workers);
    for (int worker = 0; worker < workers; ++worker)
        for (int hour = hours - 1; hour >= checkpoints.releases[worker]; --hour) {
            auto& before = need[worker][hour];
            before = need[worker][hour + 1];
            const auto service = services[worker][hour];
            if (service.kind == 0) {
                const auto& task = data.tasks[service.value];
                if (task.output >= 0) before[task.output] = std::max(Count(0), before[task.output] - task.quantity);
                if (task.input >= 0) ++before[task.input];
            } else if (service.kind == 1) before[service.value] = 0;
        }
    std::vector<ds::TileWorkAction> actions;
    for (const auto& work : problem.tile_work)
        actions.insert(actions.end(), work.actions.begin(), work.actions.end());
    require(actions.size() == data.tasks.size(), "task/action mapping differs");
    Inventory shed = problem.start.shed;
    std::vector<Inventory> cargo(workers), returned_early(workers);
    auto add = [](Count& destination, Count amount) {
        if (amount < 0 || destination > std::numeric_limits<Count>::max() - amount) return false;
        destination += amount;
        return true;
    };
    for (int hour = 0; hour < hours; ++hour) {
        for (int worker = 0; worker < schedule[hour].n_units; ++worker) {
            const auto service = services[worker][hour];
            auto& action = schedule[hour].units[worker];
            auto& carried = cargo[worker];
            if (service.kind == 0) {
                const auto& task = data.tasks[service.value];
                const auto& supplied = actions[service.value];
                if (task.input >= 0) {
                    if (!carried[task.input]) return fail("route cargo not ready");
                    --carried[task.input];
                }
                if (task.output >= 0 && !add(carried[task.output], task.quantity)) return fail("cargo overflow");
                action = {supplied.op, static_cast<uint8_t>(std::max(0, int(supplied.arg))), 1};
            } else if (service.kind == 1) {
                const int item = service.value;
                const Count quantity = screen.extra_pickups ? selected_pickups.at({worker, hour}) :
                    std::max(Count(0), need[worker][hour + 1][item] - carried[item]);
                if (!quantity) continue;
                if (quantity > shed[item] || quantity > ds::MAX_INPUT_COUNT)
                    return fail("pickup stock not ready: hour=" + std::to_string(hour) +
                        " worker=" + std::to_string(worker) + " item=" + std::to_string(item) +
                        " required=" + std::to_string(quantity) + " available=" + std::to_string(shed[item]));
                shed[item] -= quantity;
                if (!add(carried[item], quantity)) return fail("pickup overflow");
                action = {kag::OP_PICKUP, static_cast<uint8_t>(item), static_cast<int32_t>(quantity)};
            } else if (service.kind == 2) {
                bool all_surplus = true, any = false;
                int selected = -1, types = 0;
                Count selected_quantity = 0;
                Inventory due{};
                for (int item = 0; item < items; ++item) {
                    const Count safe = std::max(Count(0), carried[item] - need[worker][hour + 1][item]);
                    all_surplus &= !carried[item] || !need[worker][hour + 1][item];
                    any |= carried[item] > 0;
                    const Count credited = std::min(planned_returns[worker][hour][item], returned_early[worker][item]);
                    returned_early[worker][item] -= credited;
                    const Count planned = planned_returns[worker][hour][item] - credited;
                    due[item] = planned;
                    if (!planned) continue;
                    if (planned > safe || planned > ds::MAX_INPUT_COUNT)
                        return fail("deposit cargo not ready");
                    ++types; selected = item; selected_quantity = planned;
                }
                if (types > 1 && all_surplus && any) {
                    for (int item = 0; item < items; ++item) {
                        // DROP may bring a later planned delivery forward.
                        // Credit it once so a subsequent return does not ask
                        // the worker to deposit the same units again.
                        returned_early[worker][item] += std::max(Count(0), carried[item] - due[item]);
                        if (!add(shed[item], carried[item])) return fail("deposit overflow");
                        carried[item] = 0;
                    }
                    action = {kag::OP_DROP, 0, 1};
                } else if (types == 1) {
                    if (!add(shed[selected], selected_quantity)) return fail("deposit overflow");
                    carried[selected] -= selected_quantity;
                    action = {kag::OP_PLACE, static_cast<uint8_t>(selected), static_cast<int32_t>(selected_quantity)};
                } else if (types > 1) return fail("deposit cargo not ready: multi-item return retains inputs");
            }
        }
        for (int item = 0; item < items; ++item) {
            const Count removed = problem.shed_availability[hour][item] - (hour ? problem.shed_availability[hour - 1][item] : 0);
            if (shed[item] < removed) return fail("availability not reached");
            shed[item] -= removed;
        }
        for (const auto& event : problem.market_plan) if (event.hour == hour) {
            auto& turn = schedule[hour];
            turn.n_orders = std::max(turn.n_orders, int(event.order_index) + 1);
            turn.orders[event.order_index] = {event.market_op, static_cast<uint8_t>(std::max(0, int(event.item))), event.quantity};
            if ((event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL) && !add(shed[event.item], event.quantity))
                return fail("purchase overflow");
        }
        schedule[hour].finalize();
    }
    const auto replay = ds::replay_schedule(problem, schedule);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied) {
        if (problem.start.shed_capacity != day_scheduler::storage::unlimited && repair_seconds > 0) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(repair_seconds));
            if (auto repaired = day_scheduler::storage::repair(problem, schedule, deadline)) return repaired;
        }
        std::string error = "strict replay rejected materialization";
        for (const auto& message : replay.errors) error += ": " + message;
        if (rejected_schedule) *rejected_schedule = schedule;
        return fail(error);
    }
    return schedule;
}
} // namespace day_native::materialized_screen
