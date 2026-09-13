#include "storage.hpp"
#include "replay.hpp"
#include <algorithm>

namespace day_scheduler::storage {
namespace ds = day_solver;

static bool valid(const ds::ReplayResult& replay) {
    return replay.requirements_satisfied && replay.invariants_satisfied &&
           replay.candidate.replay.strict_valid && replay.errors.empty();
}

std::optional<std::array<kag::Action, ds::HOURS>> repair(
        const ds::DayProblem& problem, const std::array<kag::Action, ds::HOURS>& initial,
        std::chrono::steady_clock::time_point deadline) {
    auto source = initial;
    auto original = ds::replay_schedule(problem, source);
    if (valid(original)) return source;
    auto time_left = [&] { return std::chrono::steady_clock::now() < deadline; };
    auto inventory_error = [&](const auto& replay) {
        ds::InventoryCount difference = 0;
        for (int item = 0; item < kag::N_ITEMS; ++item)
            difference += std::abs(replay.candidate.end.physical.shed[item] - problem.end_shed[item]);
        return difference;
    };
    auto valid_except_inventory = [&](const auto& actions, const auto& replay) {
        if (!replay.candidate.replay.strict_valid || !replay.invariants_satisfied) return false;
        auto relaxed = problem;
        relaxed.end_shed = replay.candidate.end.physical.shed;
        for (auto& bound : relaxed.required_outcomes)
            if (bound.key.metric == ds::OutcomeMetric::END_SHED)
                bound.lower = bound.upper = relaxed.end_shed[bound.key.subject];
        return valid(ds::replay_schedule(relaxed, actions));
    };
    auto frames = trace(problem, source);
    std::array<bool, kag::N_ITEMS> needed{};
    std::array<ds::InventoryCount, kag::N_ITEMS> surplus{};
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        needed[item] = original.candidate.end.physical.shed[item] < problem.end_shed[item];
        surplus[item] = std::max(ds::InventoryCount(0), original.candidate.end.physical.shed[item] - problem.end_shed[item]);
    }
    auto try_action = [&](int hour, int worker, const kag::UnitAction& action)
            -> std::optional<std::array<kag::Action, ds::HOURS>> {
        auto candidate = source;
        candidate[hour].units[worker] = action;
        candidate[hour].finalize();
        if (valid(ds::replay_schedule(problem, candidate))) return candidate;
        return {};
    };
    // A minimum-input pickup may leave no room for a later fixed purchase.
    // Reuse an existing visit to carry spare stock; full replay checks its
    // later deposits, every purchase and the exact night inventory.
    int failed_purchase = ds::HOURS;
    for (int hour = 0; hour < ds::HOURS; ++hour) {
        ds::InventoryCount expected = 0, received = 0;
        for (const auto& order : problem.market_plan)
            if (order.hour == hour && (order.market_op == kag::M_BUY_PRODUCT || order.market_op == kag::M_BUY_ANIMAL))
                expected += order.quantity;
        const auto& state = original.candidate.hours[hour];
        for (int item = 0; item < kag::N_ITEMS; ++item)
            received += state.shed_after_market[item] - state.shed_after_workers[item];
        if (received < expected) { failed_purchase = hour; break; }
    }
    if (failed_purchase < ds::HOURS)
        for (int hour = failed_purchase; hour >= 0 && time_left(); --hour)
            for (int worker = int(frames[hour].workers.size()) - 1; worker >= 0 && time_left(); --worker) {
                const auto action = source[hour].units[worker];
                if (action.op != kag::OP_PICKUP) continue;
                const auto maximum = std::min(frames[hour].shed[action.arg], ds::MAX_INPUT_COUNT);
                for (ds::InventoryCount quantity = ds::InventoryCount(action.n) + 1; quantity <= maximum && time_left(); ++quantity)
                    if (auto result = try_action(hour, worker, {kag::OP_PICKUP, action.arg, int32_t(quantity)})) return result;
            }
    // Keep all service times and routes; use a spare action to deposit a good
    // which the current automatic night transfer loses.
    for (int hour = ds::HOURS - 1; hour >= 0 && time_left(); --hour)
        for (int worker = 0; worker < int(frames[hour].workers.size()) && time_left(); ++worker) {
            const auto& state = frames[hour].workers[worker];
            const auto op = source[hour].units[worker].op;
            if (!kag::is_shed_adjacent(state.x, state.y, kag::BOARD) ||
                op == kag::OP_PICKUP || op > kag::OP_DROP) continue;
            for (int item = 0; item < kag::N_ITEMS && time_left(); ++item)
                if (needed[item] && state.cargo[item] > 0) {
                    const auto quantity = std::min(state.cargo[item], ds::MAX_INPUT_COUNT);
                    if (auto result = try_action(hour, worker, {kag::OP_PLACE, uint8_t(item), int32_t(quantity)})) return result;
                }
            // Keeping an excess good on a later worker can leave room for a
            // required good during the automatic transfer in worker order.
            for (int item = 0; item < kag::N_ITEMS && time_left(); ++item)
                if (surplus[item] > 0 && frames[hour].shed[item] > 0) {
                    const auto quantity = std::min(surplus[item], frames[hour].shed[item]);
                    if (auto result = try_action(hour, worker, {kag::OP_PICKUP, uint8_t(item), int32_t(quantity)})) return result;
                }
        }
    // Exchange remaining routes between co-located workers to change automatic
    // night transfer order without adding actions. Full replay checks that each
    // worker still carries the inputs needed by its new route.
    for (int round = 0; round < 4 && time_left(); ++round) {
      bool improved = false;
      for (int hour = 0; hour < ds::HOURS && time_left() && !improved; ++hour)
        for (int a = 0; a < int(frames[hour].workers.size()) && time_left() && !improved; ++a)
            for (int b = a + 1; b < int(frames[hour].workers.size()) && time_left() && !improved; ++b) {
                const auto& left = frames[hour].workers[a];
                const auto& right = frames[hour].workers[b];
                if (left.x != right.x || left.y != right.y) continue;
                auto candidate = source;
                for (int h = hour; h < ds::HOURS; ++h) {
                    std::swap(candidate[h].units[a], candidate[h].units[b]);
                    candidate[h].finalize();
                }
                auto replay = ds::replay_schedule(problem, candidate);
                if (valid(replay)) return candidate;
                // Several independent cargo swaps may be required. Keep an
                // improving swap only when work, sales and all other state
                // requirements already pass; never trade them for inventory.
                if (inventory_error(replay) < inventory_error(original) && valid_except_inventory(candidate, replay)) {
                    source = std::move(candidate); original = std::move(replay);
                    improved = true;
                }
            }
      if (!improved) break;
      frames = trace(problem, source);
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        needed[item] = original.candidate.end.physical.shed[item] < problem.end_shed[item];
    // After a worker's final service and the last hire, spare movement may be
    // redirected to a shed. Every candidate still passes complete strict replay.
    int last_hire = -1;
    for (const auto& event : problem.market_plan)
        if (event.market_op == kag::M_HIRE) last_hire = std::max(last_hire, int(event.hour));
    for (int worker = 0; worker < problem.worker_count && time_left(); ++worker) {
        int first = std::max(0, last_hire + 1);
        for (int hour = 0; hour < ds::HOURS; ++hour)
            if (worker < source[hour].n_units && source[hour].units[worker].op > kag::OP_WEST)
                first = std::max(first, hour + 1);
        if (first >= ds::HOURS || worker >= int(frames[first].workers.size())) continue;
        const auto& state = frames[first].workers[worker];
        for (int item = 0; item < kag::N_ITEMS && time_left(); ++item) {
            if (!needed[item] || state.cargo[item] <= 0) continue;
            for (const auto target : {std::array{4, 4}, std::array{5, 4}, std::array{4, 5}, std::array{5, 5}}) {
                if (!time_left()) break;
                auto candidate = source;
                int x = state.x, y = state.y, hour = first;
                if (std::abs(x - target[0]) + std::abs(y - target[1]) + first >= ds::HOURS) continue;
                for (int h = first; h < ds::HOURS; ++h) candidate[h].units[worker] = {};
                while (x != target[0]) {
                    const int step = x < target[0] ? 1 : -1;
                    candidate[hour++].units[worker] = {step > 0 ? kag::OP_EAST : kag::OP_WEST, 0, 1};
                    x += step;
                }
                while (y != target[1]) {
                    const int step = y < target[1] ? 1 : -1;
                    candidate[hour++].units[worker] = {step > 0 ? kag::OP_SOUTH : kag::OP_NORTH, 0, 1};
                    y += step;
                }
                candidate[hour].units[worker] = {kag::OP_PLACE, uint8_t(item),
                    int32_t(std::min(state.cargo[item], ds::MAX_INPUT_COUNT))};
                for (auto& action : candidate) action.finalize();
                if (valid(ds::replay_schedule(problem, candidate))) return candidate;
            }
        }
    }
    return {};
}
}
