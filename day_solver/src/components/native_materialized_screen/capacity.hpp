#pragma once
#include "model.hpp"
#include "cargo_order.hpp"
#include <limits>

namespace day_native::materialized_screen {

// Choose route identities and returns to retain the requested night inventory.
// The cheap proposal and the model with cargo chronology both require replay.
inline void add_terminal_capacity(ScreenModel& screen, bool exact_order = true) {
    const auto& data = screen.data;
    const Count capacity = data.problem.start.shed_capacity;
    require(capacity != std::numeric_limits<int16_t>::max(), "terminal capacity needs a bounded shed");
    auto& model = screen.model;
    const int routes = data.route_ids.size(), workers = data.problem.worker_count;
    std::vector<std::array<Count, items>> net(routes);
    for (const auto& task : data.tasks) {
        if (task.input >= 0) --net[data.owner[task.id]][task.input];
        if (task.output >= 0) net[data.owner[task.id]][task.output] += task.quantity;
    }
    std::map<Key, LinearExpr> cargo;
    std::array<LinearExpr, items> final_stock;
    for (int item = 0; item < items; ++item)
        final_stock[item] = data.problem.start.shed[item] + data.bought(item, hours) - data.problem.shed_availability.back()[item];
    for (int route = 0; route < routes; ++route) {
        for (int item = 0; item < items; ++item) cargo[{route, item}] = net[route][item];
        for (const auto& [item, consumers] : screen.pickup_requirements.at(route)) {
            const Key key{route, item};
            const Count maximum = screen.extra_pickups ? screen.pickup_limit.at(key) : Count(consumers.size());
            const LinearExpr quantity = screen.extra_pickups ? LinearExpr(screen.pickup_units.at(key)) : LinearExpr(maximum);
            net[route][item] += maximum;
            cargo[key] += quantity;
            final_stock[item] -= quantity;
        }
    }
    for (const auto& [key, quantity] : screen.delivered_units) {
        const int task = std::get<0>(key), route = data.owner[task], item = data.tasks[task].output;
        cargo[{route, item}] -= quantity; final_stock[item] += quantity;
    }
    LinearExpr total;
    for (int item = 0; item < items; ++item) {
        model.AddGreaterOrEqual(final_stock[item], 0); total += final_stock[item];
    }
    auto load = integer(model, 0, capacity, "night_initial_load");
    model.AddEquality(load, total);
    for (const auto& [key, amount] : cargo) model.AddGreaterOrEqual(amount, 0);
    std::array<Count, items> conserved = data.problem.start.shed;
    for (int item = 0; item < items; ++item)
        conserved[item] += data.bought(item, hours) - data.problem.shed_availability.back()[item];
    for (const auto& task : data.tasks) {
        if (task.input >= 0) --conserved[task.input];
        if (task.output >= 0) conserved[task.output] += task.quantity;
    }
    const bool track_order = conserved != data.problem.end_shed;
    std::map<Key, IntVar> prefixes;
    if (exact_order) prefixes = cargo_prefixes(screen, net, cargo, track_order);
    if (!track_order) {
        // With no discarded goods, all nonnegative cargo fits at night in any
        // order. This also removes the cheap proposal's redundant night chain.
        // Exact mode retains daytime capacity and physical DROP checks above;
        // every materialized proposal still needs strict replay.
        model.AddLessOrEqual(LinearExpr(std::accumulate(conserved.begin(), conserved.end(), Count(0))), capacity);
        for (int item = 0; item < items; ++item) {
            LinearExpr retained = final_stock[item];
            for (int route = 0; route < routes; ++route) retained += cargo.at({route, item});
            model.AddEquality(retained, data.problem.end_shed[item]);
        }
        return;
    }
    if (!exact_order) {
        // Cheap proposal ordering only. Physical replay must accept it; a
        // rejected materialization can be retried with actual cargo chronology.
        int serial = 0;
        for (int worker = 0; worker < workers; ++worker)
            for (int item = 0; item < items; ++item) {
                LinearExpr carried; Count maximum = 0;
                for (int route = 0; route < routes; ++route) {
                    if (net[route][item] <= 0) continue;
                    auto amount = integer(model, 0, net[route][item], name("night_cargo", {worker, route, item}));
                    const auto assigned = screen.route_worker.at({route, worker});
                    model.AddEquality(amount, cargo.at({route, item})).OnlyEnforceIf(assigned);
                    model.AddEquality(amount, 0).OnlyEnforceIf(assigned.Not());
                    carried += amount; maximum = std::max(maximum, net[route][item]);
                }
                if (!maximum) continue;
                auto accepted = integer(model, 0, std::min(capacity, maximum), name("night_accepted", {worker, item}));
                model.AddMinEquality(accepted, {carried, capacity - load});
                auto next = integer(model, 0, capacity, name("night_load", {serial++}));
                model.AddEquality(next, load + accepted); load = next; final_stock[item] += accepted;
            }
        for (int item = 0; item < items; ++item) model.AddEquality(final_stock[item], data.problem.end_shed[item]);
        return;
    }
    for (int worker = 0; worker < workers; ++worker) {
        LinearExpr received;
        for (int item = 0; item < items; ++item) {
            LinearExpr carried, preceding; Count maximum = 0;
            for (int route = 0; route < routes; ++route) {
                if (net[route][item] <= 0) continue;
                auto amount = integer(model, 0, net[route][item], name("night_cargo", {worker, route, item}));
                const auto assigned = screen.route_worker.at({route, worker});
                model.AddEquality(amount, cargo.at({route, item})).OnlyEnforceIf(assigned);
                model.AddEquality(amount, 0).OnlyEnforceIf(assigned.Not());
                carried += amount; maximum = std::max(maximum, net[route][item]);
                const Count prefix_limit = std::accumulate(net[route].begin(), net[route].end(), Count(0)) - net[route][item];
                auto prefix = integer(model, 0, prefix_limit, name("night_prefix", {worker, route, item}));
                model.AddEquality(prefix, prefixes.at({route, item})).OnlyEnforceIf(assigned);
                model.AddEquality(prefix, 0).OnlyEnforceIf(assigned.Not());
                preceding += prefix;
            }
            if (!maximum) continue;
            auto room = integer(model, 0, capacity, name("night_room", {worker, item}));
            model.AddMaxEquality(room, {model.NewConstant(0), capacity - load - preceding});
            auto accepted = integer(model, 0, std::min(capacity, maximum), name("night_accepted", {worker, item}));
            model.AddMinEquality(accepted, {carried, room});
            final_stock[item] += accepted; received += accepted;
        }
        auto next = integer(model, 0, capacity, name("night_load", {worker}));
        model.AddEquality(next, load + received); load = next;
    }
    for (int item = 0; item < items; ++item) model.AddEquality(final_stock[item], data.problem.end_shed[item]);
}
} // namespace day_native::materialized_screen
