#pragma once
#include "bounds.hpp"
#include <limits>

namespace labor {
inline constexpr std::array<const char*, 10> route_feature_names = {
    "optimized_open_workers", "optimized_return_workers", "optimized_open_effort", "optimized_return_effort",
    "optimized_open_slack", "optimized_return_slack", "optimized_tour_open_distance", "optimized_tour_closed_distance",
    "optimized_orders_tested", "optimized_route_failed"};
using RouteFeatures = std::array<float, route_feature_names.size()>;
namespace route_detail {
struct Job { int cell = 0, work = 0; unsigned inputs = 0; bool output = false; };
using Jobs = std::array<Job, 100>;
using Order = std::array<int, 100>;
struct Split { int workers = 41, effort = 0, slack = 0; };

inline int tour_distance(const Jobs& jobs, const Order& order, int n, bool closed) {
    int result = shed_distance(jobs[order[0]].cell);
    for (int i = 1; i < n; ++i) result += distance(jobs[order[i - 1]].cell, jobs[order[i]].cell);
    return result + (closed ? shed_distance(jobs[order[n - 1]].cell) : 0);
}

inline Order initial_order(const Jobs& jobs, int n, int seed) {
    Order order{};
    for (int i = 0; i < n; ++i) order[i] = i;
    if (seed < 4) {
        auto rank = [&](int index) {
            int x = jobs[index].cell % 10, y = jobs[index].cell / 10;
            if (seed & 1) std::swap(x, y);
            if (seed & 2) x = 9 - x;
            return y * 10 + ((y & 1) ? 9 - x : x);
        };
        std::sort(order.begin(), order.begin() + n, [&](int a, int b) { return rank(a) < rank(b); });
    } else {
        std::array<bool, 100> used{};
        int first = 0;
        for (int i = 1; i < n; ++i)
            if (seed == 4 ? shed_distance(jobs[i].cell) < shed_distance(jobs[first].cell)
                          : shed_distance(jobs[i].cell) > shed_distance(jobs[first].cell)) first = i;
        order[0] = first; used[first] = true;
        for (int position = 1; position < n; ++position) {
            int best = -1, closest = 100;
            for (int i = 0; i < n; ++i) if (!used[i]) {
                const int d = distance(jobs[order[position - 1]].cell, jobs[i].cell);
                if (d < closest) { best = i; closest = d; }
            }
            order[position] = best; used[best] = true;
        }
    }
    return order;
}

inline void improve(const Jobs& jobs, Order& order, int n, bool closed) {
    for (int pass = 0; pass < 8; ++pass) {
        int saving = 0, left = -1, right = -1;
        for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) {
            const int a = jobs[order[i]].cell, b = jobs[order[j]].cell;
            const int previous = i ? jobs[order[i - 1]].cell : -1;
            const int next = j + 1 < n ? jobs[order[j + 1]].cell : -1;
            const int before = (previous < 0 ? shed_distance(a) : distance(previous, a))
                + (next < 0 ? (closed ? shed_distance(b) : 0) : distance(b, next));
            const int after = (previous < 0 ? shed_distance(b) : distance(previous, b))
                + (next < 0 ? (closed ? shed_distance(a) : 0) : distance(a, next));
            if (before - after > saving) { saving = before - after; left = i; right = j; }
        }
        if (!saving) break;
        std::reverse(order.begin() + left, order.begin() + right + 1);
    }
}

inline Split split(const Jobs& jobs, const Order& order, int n, bool closed, const HireMenu& menu, int hours) {
    constexpr int inf = 1000000;
    std::array<std::array<int16_t, 101>, 100> segment{};
    std::array<int, 100> farmer_extra{};
    for (int i = 0; i < n; ++i) {
        unsigned inputs = 0; bool output = false;
        int effort = shed_distance(jobs[order[i]].cell), last = -1;
        farmer_extra[i] = distance(44, jobs[order[i]].cell) - effort;
        for (int j = i; j < n; ++j) {
            const auto& job = jobs[order[j]];
            effort += job.work + (last < 0 ? 0 : distance(last, job.cell));
            last = job.cell; inputs |= job.inputs; output |= job.output;
            const int total = effort + std::popcount(inputs) + (closed && output ? shed_distance(last) + 1 : 0);
            segment[i][j + 1] = std::min(total, 30000);
        }
    }
    std::array<int, 101> before{}, after{}; before.fill(inf); before[0] = 0;
    int capacity = 0;
    for (int worker = 1; worker <= 40; ++worker) {
        const int room = worker == 1 ? hours : hours - 1 - menu.hours[worker - 2];
        capacity += room;
        after = before; // A worker may be idle, including the farmer.
        for (int begin = 0; begin < n; ++begin) if (before[begin] < inf) {
            for (int end = begin + 1; end <= n; ++end) {
                const int effort = segment[begin][end] + (worker == 1 ? farmer_extra[begin] : 0);
                if (effort > room) break;
                after[end] = std::min(after[end], before[begin] + effort);
            }
        }
        if (after[n] < inf) return {worker, after[n], capacity - after[n]};
        before = after;
    }
    return {};
}
}

// Structural routing features, not a valid game schedule or a lower bound.
// Whole chains remain on one worker, inputs are assumed available at departure,
// and each hired worker may start at its best shed square. Inventory timing,
// intermediate deliveries and splitting a chain across workers are omitted.
inline RouteFeatures optimized_routes(const day_solver::DayProblem& p, int active_hours = 24) {
    if (active_hours != 23 && active_hours != 24) throw std::runtime_error("unsupported active horizon");
    using namespace route_detail;
    Jobs jobs{}; int n = 0;
    for (const auto& work : p.tile_work) {
        if (work.actions.empty()) continue;
        const auto& tile = p.start.managed_tiles[work.tile];
        auto& job = jobs[n++]; job.cell = tile.y * 10 + tile.x; job.work = work.actions.size();
        std::array<int64_t, kag::N_ITEMS> balance{};
        for (const auto& action : work.actions) {
            int item = -1;
            if (action.op == kag::OP_FEED) item = kag::WHEAT;
            if (action.op == kag::OP_FERTILIZE) item = kag::FERTILIZER;
            if (action.op == kag::OP_PLACE) item = action.arg;
            if (item >= 0) { if (balance[item]) --balance[item]; else job.inputs |= 1u << item; }
            if (action.output_item >= 0) balance[action.output_item] += action.output_quantity;
        }
        job.output = std::any_of(balance.begin(), balance.end(), [](auto value) { return value > 0; });
    }
    if (!n) return {1, 1, 0, 0, float(active_hours), float(active_hours), 0, 0, 0, 0};
    std::sort(jobs.begin(), jobs.begin() + n, [](const auto& a, const auto& b) { return a.cell < b.cell; });
    const auto menu = earliest_menu(p, active_hours);
    std::array<Order, 12> seen{}; int seen_count = 0;
    std::array<Split, 2> best{}; std::array<int, 2> best_distance{1000000, 1000000};
    for (int closed = 0; closed < 2; ++closed) for (int seed = 0; seed < 6; ++seed) {
        auto order = initial_order(jobs, n, seed); improve(jobs, order, n, closed);
        bool duplicate = false;
        for (int i = 0; i < seen_count; ++i) duplicate |= std::equal(order.begin(), order.begin() + n, seen[i].begin());
        if (duplicate) continue;
        seen[seen_count++] = order;
        for (int mode = 0; mode < 2; ++mode) {
            best_distance[mode] = std::min(best_distance[mode], tour_distance(jobs, order, n, mode));
            const auto result = split(jobs, order, n, mode, menu, active_hours);
            if (std::pair(result.workers, result.effort) < std::pair(best[mode].workers, best[mode].effort)) best[mode] = result;
        }
    }
    if (best[0].workers > best[1].workers) throw std::runtime_error("relaxed open route count exceeds closed count");
    return {float(best[0].workers), float(best[1].workers), float(best[0].effort), float(best[1].effort),
            float(best[0].slack), float(best[1].slack), float(best_distance[0]), float(best_distance[1]),
            float(seen_count), float(best[0].workers > 40 || best[1].workers > 40)};
}
}
