#pragma once
#include "model.hpp"

namespace day_native::materialized_screen {

// Inventory constraints for the fixed-order constructor. Each route picks up
// an item's unmet demand once and keeps locally needed production. This is a
// restricted constructor, not a proof that other physical schedules cannot exist.
inline void add_stock_constraints(ScreenModel& screen, bool bounded = false) {
    const auto& data = screen.data;
    auto& model = screen.model;
    require(data.options.fixed_order && data.options.free_order.empty(), "stock construction requires fixed task order");
    struct Transfer {
        int item;
        LinearExpr event;
        LinearExpr quantity;
        Count maximum;
        bool pickup;
        IntVar time;
        int route;
    };
    std::vector<Transfer> transfers;
    const int stride = data.problem.worker_count + 1;
    for (int route = 0; route < int(screen.route_nodes.size()); ++route) {
        auto order = data.hinted[route];
        std::sort(order.begin(), order.end());
        std::array<Count, items> needed{};
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            const auto& task = data.tasks[it->second];
            if (task.output >= 0) {
                const Count reserved = std::min(needed[task.output], task.quantity);
                needed[task.output] -= reserved;
                LinearExpr returned;
                for (const auto& [key, units] : screen.delivered_units)
                    if (std::get<0>(key) == task.id) returned += units;
                model.AddLessOrEqual(returned, task.quantity - reserved);
            }
            if (task.input >= 0) ++needed[task.input];
        }
        for (const auto& [item, consumers] : screen.pickup_requirements.at(route)) {
            require(!consumers.empty(), "fixed-order pickup has no demand");
            auto time = integer(model, 0, hours - 1, name("stock_pickup_time", {route, item}));
            for (const auto& node : screen.route_nodes[route])
                if (node.kind == 1 && node.value == item) model.AddEquality(time, node.time).OnlyEnforceIf(node.active);
            const Key key{route, item};
            const Count maximum = screen.extra_pickups ? screen.pickup_limit.at(key) : Count(consumers.size());
            const LinearExpr quantity = screen.extra_pickups ? LinearExpr(screen.pickup_units.at(key)) : LinearExpr(maximum);
            transfers.push_back({item, time * stride + screen.concrete_worker[route], quantity, maximum, true, time, route});
        }
    }
    std::map<std::tuple<int, int, int, int>, LinearExpr> deposits;
    std::map<std::tuple<int, int, int, int>, Count> maximum;
    for (const auto& [key, units] : screen.delivered_units) {
        const auto [task, deadline, access] = key;
        const auto grouped = std::tuple(data.owner[task], deadline, access, data.tasks[task].output);
        deposits[grouped] += units;
        maximum[grouped] += data.tasks[task].quantity;
    }
    for (const auto& [key, quantity] : deposits) {
        const auto [route, deadline, access, item] = key;
        const auto time = screen.deposit_time.at({route, deadline, access});
        transfers.push_back({item, time * stride + screen.concrete_worker[route], quantity, maximum.at(key), false, time, route});
    }
    int serial = 0;
    auto contribution = [&](const Transfer& transfer, LinearExpr checkpoint) -> LinearExpr {
        const int id = serial++;
        auto before = boolean(model, name("stock_before", {id}));
        model.AddLessThan(transfer.event, checkpoint).OnlyEnforceIf(before);
        model.AddGreaterOrEqual(transfer.event, checkpoint).OnlyEnforceIf(before.Not());
        if (transfer.pickup && !screen.extra_pickups) return before * transfer.maximum;
        auto amount = integer(model, 0, transfer.maximum, name("stock_returned", {id}));
        model.AddEquality(amount, transfer.quantity).OnlyEnforceIf(before);
        model.AddEquality(amount, 0).OnlyEnforceIf(before.Not());
        return amount;
    };
    for (const auto& pickup : transfers) {
        if (!pickup.pickup) continue;
        std::vector<int64_t> supply;
        for (int hour = 0; hour < hours; ++hour)
            supply.push_back(data.problem.start.shed[pickup.item] + data.bought(pickup.item, hour) -
                (hour ? data.problem.shed_availability[hour - 1][pickup.item] : 0));
        auto stock = integer(model, *std::min_element(supply.begin(), supply.end()),
            *std::max_element(supply.begin(), supply.end()), name("stock_at_pickup", {serial++}));
        model.AddElement(pickup.time, supply, stock);
        LinearExpr balance = stock - pickup.quantity;
        for (const auto& transfer : transfers) {
            if (&transfer == &pickup || transfer.item != pickup.item) continue;
            const auto amount = contribution(transfer, pickup.event);
            balance += transfer.pickup ? -amount : amount;
        }
        model.AddGreaterOrEqual(balance, 0);
    }
    if (bounded) {
        const int routes = data.route_ids.size(), workers = data.problem.worker_count;
        std::vector<std::array<LinearExpr, hours>> changes(routes);
        std::vector<Count> incoming(routes), outgoing(routes);
        std::map<Key, BoolVar> active_at;
        for (const auto& transfer : transfers) {
            if (transfer.pickup) outgoing[transfer.route] += transfer.maximum;
            for (int hour = 0; hour < hours; ++hour) {
                const Key key{transfer.time.index(), hour};
                if (!active_at.contains(key)) {
                    auto active = boolean(model, name("shed_event_at", {serial++}));
                    model.AddEquality(transfer.time, hour).OnlyEnforceIf(active);
                    model.AddNotEqual(transfer.time, hour).OnlyEnforceIf(active.Not());
                    active_at.emplace(key, active);
                }
                const auto active = active_at.at(key);
                if (transfer.pickup && !screen.extra_pickups) changes[transfer.route][hour] -= active * transfer.maximum;
                else {
                    auto amount = integer(model, 0, transfer.maximum, name("shed_return_at", {serial++}));
                    model.AddEquality(amount, transfer.quantity).OnlyEnforceIf(active);
                    model.AddEquality(amount, 0).OnlyEnforceIf(active.Not());
                    changes[transfer.route][hour] += transfer.pickup ? -LinearExpr(amount) : LinearExpr(amount);
                }
            }
        }
        for (const auto& task : data.tasks) if (task.output >= 0) incoming[data.owner[task.id]] += task.quantity;
        std::vector<std::array<IntVar, hours>> route_change(routes);
        for (int route = 0; route < routes; ++route)
            for (int hour = 0; hour < hours; ++hour) {
                auto change = integer(model, -outgoing[route], incoming[route], name("route_shed_change", {route, hour}));
                model.AddEquality(change, changes[route][hour]); route_change[route][hour] = change;
            }
        const Count low = outgoing.empty() ? 0 : -*std::max_element(outgoing.begin(), outgoing.end());
        const Count high = incoming.empty() ? 0 : *std::max_element(incoming.begin(), incoming.end());
        std::vector<BoolVar> working;
        for (int worker = 0; worker < workers; ++worker) {
            LinearExpr assigned;
            for (int route = 0; route < routes; ++route) assigned += screen.route_worker.at({route, worker});
            auto active = boolean(model, name("has_shed_route", {worker}));
            model.AddEquality(active, assigned); working.push_back(active);
        }
        IntVar load = model.NewConstant(std::accumulate(data.problem.start.shed.begin(), data.problem.start.shed.end(), Count(0)));
        for (int hour = 0; hour < hours; ++hour) {
            for (int worker = 0; worker < workers; ++worker) {
                auto change = integer(model, low, high, name("worker_shed_change", {worker, hour}));
                model.AddEquality(change, 0).OnlyEnforceIf(working[worker].Not());
                for (int route = 0; route < routes; ++route)
                    model.AddEquality(change, route_change[route][hour]).OnlyEnforceIf(screen.route_worker.at({route, worker}));
                auto next = integer(model, 0, data.problem.start.shed_capacity, name("shed_load_after_worker", {worker, hour}));
                model.AddEquality(next, load + change); load = next;
            }
            Count market = 0;
            for (int item = 0; item < items; ++item)
                market += data.bought(item, hour + 1) - data.bought(item, hour) - data.problem.shed_availability[hour][item] +
                    (hour ? data.problem.shed_availability[hour - 1][item] : 0);
            auto next = integer(model, 0, data.problem.start.shed_capacity, name("shed_load_after_market", {hour}));
            model.AddEquality(next, load + market); load = next;
        }
    }
    for (int item = 0; item < items; ++item) {
        const bool picked = std::any_of(transfers.begin(), transfers.end(), [&](const auto& t) { return t.item == item && t.pickup; });
        if (!picked) continue;  // Existing delivery constraints cover pure outputs.
        for (int hour = 0; hour < hours; ++hour) {
            if (hour != hours - 1 && data.problem.shed_availability[hour][item] ==
                (hour ? data.problem.shed_availability[hour - 1][item] : 0)) continue;
            LinearExpr balance = data.problem.start.shed[item] + data.bought(item, hour) - data.problem.shed_availability[hour][item];
            for (const auto& transfer : transfers) {
                if (transfer.item != item) continue;
                const auto amount = contribution(transfer, hour * stride + data.problem.worker_count);
                balance += transfer.pickup ? -amount : amount;
            }
            model.AddGreaterOrEqual(balance, 0);
        }
    }
}
} // namespace day_native::materialized_screen
