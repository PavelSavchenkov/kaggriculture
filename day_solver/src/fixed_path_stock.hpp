#pragma once
#include "day_solver/io.hpp"
#include "day_solver/scheduler.hpp"
#include "components/native_solver_api/internal_hint.hpp"
#include "storage.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"

namespace fixed_path_stock {
namespace ds = day_solver;
namespace sat = operations_research::sat;
using Count = ds::InventoryCount;

inline day_scheduler::Result solve(const ds::DayProblem& problem,
        const std::array<kag::Action, ds::HOURS>& original,
        const day_native::InternalHint& hint, double seconds, bool spare_transfers = false) {
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    sat::CpModelBuilder model;
    auto integer = [&](Count low, Count high) { return model.NewIntVar(operations_research::Domain(low, high)); };
    const int workers = problem.worker_count, items = kag::N_ITEMS;
    const Count capacity = problem.start.shed_capacity;
    const auto frames = day_scheduler::storage::trace(problem, original);
    std::vector<ds::TileWorkAction> tasks;
    for (const auto& work : problem.tile_work) tasks.insert(tasks.end(), work.actions.begin(), work.actions.end());
    std::map<std::pair<int, int>, int> task_at;
    for (const auto& row : hint.assignments) task_at[{row.hour.value(), row.worker}] = row.task;
    std::array<Count, kag::N_ITEMS> maximum = problem.start.shed;
    for (const auto& event : problem.market_plan)
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)
            maximum[event.item] += event.quantity;
    for (const auto& task : tasks) if (task.output_item >= 0) maximum[task.output_item] += task.output_quantity;
    std::array<sat::IntVar, kag::N_ITEMS> shed;
    std::vector<std::array<sat::IntVar, kag::N_ITEMS>> cargo(workers), origin(workers);
    std::vector<std::array<Count, kag::N_ITEMS>> upper(workers);
    for (int item = 0; item < items; ++item) {
        shed[item] = model.NewConstant(problem.start.shed[item]);
        for (int worker = 0; worker < workers; ++worker)
            cargo[worker][item] = origin[worker][item] = model.NewConstant(0);
    }
    auto shed_load = [&] {
        sat::LinearExpr load;
        for (auto item : shed) load += item;
        return load;
    };
    auto change_shed = [&](int item, sat::LinearExpr delta) {
        auto next = integer(0, capacity);
        model.AddEquality(next, shed[item] + delta);
        shed[item] = next;
    };
    auto change_cargo = [&](int worker, int item, sat::LinearExpr delta, bool acquired, int hour) {
        auto next = integer(0, maximum[item]);
        model.AddEquality(next, cargo[worker][item] + delta);
        if (acquired) {
            auto empty = model.NewBoolVar();
            model.AddEquality(cargo[worker][item], 0).OnlyEnforceIf(empty);
            model.AddGreaterThan(cargo[worker][item], 0).OnlyEnforceIf(empty.Not());
            auto inserted = integer(0, hour);
            model.AddEquality(inserted, hour).OnlyEnforceIf(empty);
            model.AddEquality(inserted, origin[worker][item]).OnlyEnforceIf(empty.Not());
            origin[worker][item] = inserted;
        }
        cargo[worker][item] = next;
    };
    auto drop = [&](int worker) {
        const auto load = shed_load();
        std::array<sat::IntVar, kag::N_ITEMS> accepted;
        for (int item = 0; item < items; ++item) {
            if (!upper[worker][item]) continue;
            sat::LinearExpr prefix;
            for (int other = 0; other < items; ++other) {
                if (other == item || !upper[worker][other]) continue;
                auto before = model.NewBoolVar();
                model.AddLessThan(origin[worker][other], origin[worker][item]).OnlyEnforceIf(before);
                model.AddGreaterOrEqual(origin[worker][other], origin[worker][item]).OnlyEnforceIf(before.Not());
                auto amount = integer(0, upper[worker][other]);
                model.AddEquality(amount, cargo[worker][other]).OnlyEnforceIf(before);
                model.AddEquality(amount, 0).OnlyEnforceIf(before.Not());
                prefix += amount;
            }
            auto room = integer(0, capacity);
            model.AddMaxEquality(room, {sat::LinearExpr(0), capacity - load - prefix});
            accepted[item] = integer(0, std::min(capacity, upper[worker][item]));
            model.AddMinEquality(accepted[item], {sat::LinearExpr(cargo[worker][item]), sat::LinearExpr(room)});
        }
        for (int item = 0; item < items; ++item) if (upper[worker][item]) {
            change_shed(item, accepted[item]);
            cargo[worker][item] = model.NewConstant(0);
            upper[worker][item] = 0;
        }
        model.AddLessOrEqual(shed_load(), capacity);
    };
    struct Transfer { int hour, worker; uint8_t op, item; sat::IntVar quantity; bool optional; };
    std::vector<Transfer> quantities;
    for (int hour = 0; hour < ds::HOURS; ++hour) {
        for (int worker = 0; worker < original[hour].n_units; ++worker) {
            const auto action = original[hour].units[worker];
            if (task_at.contains({hour, worker})) {
                const auto task = tasks[task_at.at({hour, worker})];
                const int input = task.op == kag::OP_FEED ? kag::WHEAT :
                    task.op == kag::OP_FERTILIZE ? kag::FERTILIZER : task.op == kag::OP_PLACE ? task.arg : -1;
                if (input >= 0) {
                    change_cargo(worker, input, sat::LinearExpr(-Count(task.quantity)), false, hour);
                    upper[worker][input] = std::max(Count(0), upper[worker][input] - task.quantity);
                }
                if (task.output_item >= 0) {
                    change_cargo(worker, task.output_item, sat::LinearExpr(task.output_quantity), true, hour);
                    upper[worker][task.output_item] += task.output_quantity;
                }
            } else if (action.op == kag::OP_PICKUP || action.op == kag::OP_PLACE) {
                const int item = action.arg;
                const bool pickup = action.op == kag::OP_PICKUP;
                const Count bound = pickup ? std::min(capacity, maximum[item]) : upper[worker][item];
                auto quantity = integer(0, bound);
                quantities.push_back({hour, worker, action.op, action.arg, quantity, false});
                model.AddHint(quantity, std::min(Count(action.n), bound));
                change_shed(item, pickup ? -sat::LinearExpr(quantity) : sat::LinearExpr(quantity));
                change_cargo(worker, item, pickup ? sat::LinearExpr(quantity) : -sat::LinearExpr(quantity), pickup, hour);
                if (pickup) upper[worker][item] = std::min(maximum[item], upper[worker][item] + bound);
                model.AddLessOrEqual(shed_load(), capacity);
            } else if (action.op == kag::OP_DROP) drop(worker);
            else if (spare_transfers && action.op == kag::OP_PASS &&
                kag::is_shed_adjacent(frames[hour].workers[worker].x, frames[hour].workers[worker].y, kag::BOARD)) {
                sat::LinearExpr chosen;
                for (int item = 0; item <= kag::FERTILIZER; ++item)
                    for (const bool pickup : {true, false}) {
                        const Count bound = pickup ? std::min(capacity, maximum[item]) : upper[worker][item];
                        if (!bound) continue;
                        auto quantity = integer(0, bound);
                        auto active = model.NewBoolVar();
                        model.AddGreaterThan(quantity, 0).OnlyEnforceIf(active);
                        model.AddEquality(quantity, 0).OnlyEnforceIf(active.Not());
                        chosen += active;
                        const uint8_t op = pickup ? kag::OP_PICKUP : kag::OP_PLACE;
                        quantities.push_back({hour, worker, op, uint8_t(item), quantity, true});
                        change_shed(item, pickup ? -sat::LinearExpr(quantity) : sat::LinearExpr(quantity));
                        change_cargo(worker, item, pickup ? sat::LinearExpr(quantity) : -sat::LinearExpr(quantity), pickup, hour);
                        if (pickup) upper[worker][item] = std::min(maximum[item], upper[worker][item] + bound);
                    }
                model.AddLessOrEqual(chosen, 1);
                model.AddLessOrEqual(shed_load(), capacity);
            }
        }
        for (int item = 0; item < items; ++item) {
            const Count removed = problem.shed_availability[hour][item] - (hour ? problem.shed_availability[hour - 1][item] : 0);
            if (removed) change_shed(item, sat::LinearExpr(-removed));
        }
        for (const auto& event : problem.market_plan)
            if (event.hour == hour && (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)) {
                change_shed(event.item, sat::LinearExpr(event.quantity));
                model.AddLessOrEqual(shed_load(), capacity);
            }
    }
    for (int worker = 0; worker < workers; ++worker) drop(worker);
    for (int item = 0; item < items; ++item) model.AddEquality(shed[item], problem.end_shed[item]);
    const double built = elapsed();
    day_scheduler::Result result;
    result.stages.push_back({"fixed_stock/build", "variables=" + std::to_string(model.Build().variables_size()), built});
    if (built < seconds) {
        sat::SatParameters parameters;
        parameters.set_max_time_in_seconds(seconds - built);
        parameters.set_num_search_workers(1);
        parameters.set_symmetry_level(0);
        parameters.set_cp_model_probing_level(0);
        sat::Model solver;
        solver.Add(sat::NewSatParameters(parameters));
        const auto response = sat::SolveCpModel(model.Build(), &solver);
        result.stages.push_back({"fixed_stock/search", sat::CpSolverStatus_Name(response.status()), response.wall_time()});
        if (response.status() == sat::OPTIMAL || response.status() == sat::FEASIBLE) {
            auto actions = original;
            for (const auto& transfer : quantities) {
                const auto amount = sat::SolutionIntegerValue(response, transfer.quantity);
                if (amount) actions[transfer.hour].units[transfer.worker] = {transfer.op, transfer.item, int32_t(amount)};
                else if (!transfer.optional) actions[transfer.hour].units[transfer.worker] = {};
            }
            for (auto& action : actions) action.finalize();
            const auto checked = ds::replay_schedule(problem, actions);
            if (checked.candidate.replay.strict_valid && checked.invariants_satisfied && checked.requirements_satisfied)
                result.schedule = std::move(actions);
            else for (const auto& error : checked.errors) result.stages.push_back({"fixed_stock/rejected", error, 0});
        }
    }
    result.seconds = elapsed();
    return result;
}
}
