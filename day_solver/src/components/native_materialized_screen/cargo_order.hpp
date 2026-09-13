#pragma once
#include "model.hpp"

namespace day_native::materialized_screen {

// Track when each nonempty cargo entry was inserted. Spending or depositing
// its last unit removes that entry; acquiring the item again appends it.
inline std::map<Key, IntVar> cargo_prefixes(ScreenModel& screen,
        const std::vector<std::array<Count, items>>& net, const std::map<Key, LinearExpr>& final_cargo, bool track_order = true) {
    auto& model = screen.model;
    const auto& data = screen.data;
    std::map<Key, BoolVar> at_hour;
    auto at = [&](IntVar time, int hour) {
        const Key key{time.index(), hour};
        if (!at_hour.contains(key)) {
            auto active = boolean(model, name("cargo_event", {time.index(), hour}));
            model.AddEquality(time, hour).OnlyEnforceIf(active);
            model.AddNotEqual(time, hour).OnlyEnforceIf(active.Not());
            at_hour.emplace(key, active);
        }
        return at_hour.at(key);
    };
    std::map<Key, IntVar> origin;
    int serial = 0;
    for (int route = 0; route < int(data.route_ids.size()); ++route) {
        for (int item = 0; item < items; ++item) {
            if (net[route][item] <= 0) continue;
            std::array<LinearExpr, hours> delta;
            Count maximum = 0;
            for (int task : data.route_tasks[route]) {
                const auto& action = data.tasks[task];
                if (action.input != item && action.output != item) continue;
                const Count change = (action.output == item ? action.quantity : 0) - (action.input == item);
                maximum += std::max(Count(0), change);
                for (int hour = data.early[task]; hour <= data.late[task]; ++hour)
                    delta[hour] += at(screen.task_time[task], hour) * change;
            }
            if (screen.pickup_requirements.at(route).contains(item)) {
                const Key key{route, item};
                const Count quantity = screen.extra_pickups ? screen.pickup_limit.at(key) : Count(screen.pickup_requirements.at(route).at(item).size());
                maximum += quantity;
                auto time = integer(model, 0, hours - 1, name("cargo_pickup", {route, item}));
                for (const auto& node : screen.route_nodes[route]) if (node.kind == 1 && node.value == item)
                    model.AddEquality(time, node.time).OnlyEnforceIf(node.active);
                for (int hour = 0; hour < hours; ++hour) {
                    if (!screen.extra_pickups) delta[hour] += at(time, hour) * quantity;
                    else {
                        auto amount = integer(model, 0, quantity, name("cargo_picked", {serial++}));
                        model.AddEquality(amount, screen.pickup_units.at(key)).OnlyEnforceIf(at(time, hour));
                        model.AddEquality(amount, 0).OnlyEnforceIf(at(time, hour).Not());
                        delta[hour] += amount;
                    }
                }
            }
            std::map<Triple, LinearExpr> returned;
            for (const auto& [key, units] : screen.delivered_units) {
                const auto [task, deadline, access] = key;
                if (data.owner[task] == route && data.tasks[task].output == item)
                    returned[{route, deadline, access}] += units;
            }
            for (const auto& [key, quantity] : returned)
                for (int hour = 0; hour < hours; ++hour) {
                    auto amount = integer(model, 0, maximum, name("cargo_return", {serial++}));
                    const auto active = at(screen.deposit_time.at(key), hour);
                    model.AddEquality(amount, quantity).OnlyEnforceIf(active);
                    model.AddEquality(amount, 0).OnlyEnforceIf(active.Not());
                    delta[hour] -= amount;
                }
            IntVar cargo = model.NewConstant(0), inserted = model.NewConstant(0);
            for (int hour = 0; hour < hours; ++hour) {
                auto next = integer(model, 0, maximum, name("cargo_load", {route, item, hour}));
                model.AddEquality(next, cargo + delta[hour]);
                // A physical DROP empties every item, including goods whose
                // delivery was optional. Keep the planned stock changes exact.
                for (const auto& [key, drop] : screen.drop_all) if (std::get<0>(key) == route)
                    model.AddEquality(next, 0).OnlyEnforceIf({drop, at(screen.deposit_time.at(key), hour)});
                if (track_order) {
                    auto empty = boolean(model, name("cargo_empty", {route, item, hour}));
                    model.AddEquality(cargo, 0).OnlyEnforceIf(empty);
                    model.AddGreaterThan(cargo, 0).OnlyEnforceIf(empty.Not());
                    auto start = integer(model, 0, hour, name("cargo_inserted", {route, item, hour}));
                    model.AddEquality(start, hour).OnlyEnforceIf(empty);
                    model.AddEquality(start, inserted).OnlyEnforceIf(empty.Not());
                    inserted = start;
                }
                cargo = next;
            }
            model.AddEquality(cargo, final_cargo.at({route, item}));
            if (track_order) origin.emplace(Key{route, item}, inserted);
        }
    }
    std::map<Key, IntVar> prefixes;
    for (const auto& [key, inserted] : origin) {
        const auto [route, item] = key;
        LinearExpr preceding;
        Count maximum = 0;
        for (int other = 0; other < items; ++other) {
            if (other == item || net[route][other] <= 0) continue;
            auto before = boolean(model, name("cargo_precedes", {route, item, other}));
            model.AddLessThan(origin.at({route, other}), inserted).OnlyEnforceIf(before);
            model.AddGreaterOrEqual(origin.at({route, other}), inserted).OnlyEnforceIf(before.Not());
            auto amount = integer(model, 0, net[route][other], name("cargo_preceding", {route, item, other}));
            model.AddEquality(amount, final_cargo.at({route, other})).OnlyEnforceIf(before);
            model.AddEquality(amount, 0).OnlyEnforceIf(before.Not());
            preceding += amount; maximum += net[route][other];
        }
        auto prefix = integer(model, 0, maximum, name("cargo_prefix", {route, item}));
        model.AddEquality(prefix, preceding); prefixes.emplace(key, prefix);
    }
    return prefixes;
}
}
