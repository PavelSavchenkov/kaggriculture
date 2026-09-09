#pragma once
#include <day_solver/scheduler.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <string>
#include <stdexcept>
#include <vector>

namespace labor {
enum Feature {
    tasks, active_tiles, max_tile_tasks, mean_tile_tasks, task_squared_sum,
    task_distance, input_distance, output_distance, distance_sum, distance_max,
    bbox_width, bbox_height, pair_distance_mean, nearest_distance_mean, rooted_mst,
    adjacent_pairs, connected_components, task_distance_std, empty_tiles, locked_tiles,
    purchase_events, purchase_last_hour, purchase_weighted_hour, purchase_units,
    initial_shed_total, end_shed_total, withdrawal_total, withdrawal_events,
    first_withdrawal_hour, last_withdrawal_hour, weighted_withdrawal_hour,
    early_withdrawal_deficit, max_chain, route_work_proxy, task_motion_bound,
    operations_base,
    input_base = operations_base + 18,
    output_base = input_base + kag::N_ITEMS,
    shed_base = output_base + kag::N_ITEMS,
    bought_base = shed_base + kag::N_ITEMS,
    seeds_base = bought_base + kag::N_ITEMS,
    deadlines_base = seeds_base + kag::N_CROPS,
    quadrant_tasks_base = deadlines_base + 5 * kag::N_ITEMS,
    quadrant_tiles_base = quadrant_tasks_base + 4,
    species_base = quadrant_tiles_base + 4,
    productive_tiles = species_base + kag::N_ITEMS,
    output_actions, output_action_distance, input_actions, input_action_distance,
    distinct_inputs, required_pickup_types, output_kept_until_night, deadline_missing_quantity,
    deadline_pressure_max, deadline_required_ops_max, late_input_pressure, locked_work_pressure,
    neighbor_work_1, neighbor_work_2, neighbor_work_3, route_pack_open, route_pack_return,
    deadline_required_actions_base,
    deadline_deficit_base = deadline_required_actions_base + 24,
    count = deadline_deficit_base + 5
};
using Features = std::array<float, count>;

inline int distance(int a, int b) {
    return std::abs(a % 10 - b % 10) + std::abs(a / 10 - b / 10);
}
inline int shed_distance(int cell) {
    return std::min(std::abs(cell % 10 - 4), std::abs(cell % 10 - 5))
         + std::min(std::abs(cell / 10 - 4), std::abs(cell / 10 - 5));
}

struct RouteTile { int cell = 0, work = 0; unsigned inputs = 0; };

// A fast routing proxy, not a feasibility test: whole tile chains stay with one
// worker, input types are picked once, and workers have23 phases each.
inline float route_pack(std::array<RouteTile, 100> tiles, int n, bool return_to_shed, int capacity = 23) {
    int best = 10000;
    for (int orientation = 0; orientation < 4; ++orientation) {
        auto rank = [&](int cell) {
            int x = cell % 10, y = cell / 10;
            if (orientation & 1) std::swap(x, y);
            if (orientation & 2) x = 9 - x;
            return y * 10 + ((y & 1) ? 9 - x : x);
        };
        std::sort(tiles.begin(), tiles.begin() + n, [&](const auto& a, const auto& b) { return rank(a.cell) < rank(b.cell); });
        int routes = 0, used = 0, last = -1; unsigned inputs = 0;
        for (int i = 0; i < n; ++i) {
            const auto& t = tiles[i];
            const int tail = return_to_shed ? shed_distance(t.cell) + 1 : 0;
            int add = t.work + (last < 0 ? shed_distance(t.cell) : distance(last, t.cell))
                    + std::popcount(t.inputs & ~inputs);
            if (used && used + add + tail > capacity) { ++routes; used = 0; last = -1; inputs = 0;
                add = t.work + shed_distance(t.cell) + std::popcount(t.inputs); }
            used += add; last = t.cell; inputs |= t.inputs;
            if (used + tail > capacity) { routes += (used + tail + capacity - 1) / capacity; used = 0; last = -1; inputs = 0; }
        }
        best = std::min(best, routes + (used > 0));
    }
    return std::max(1, best);
}

inline void add_pressure_features(const day_solver::DayProblem& p, Features& f, int active_hours = 24) {
    std::array<std::array<double, kag::N_ITEMS>, 9> output_by_distance{};
    std::array<double, kag::N_ITEMS> max_yield{}, produced{}, required_input{};
    std::array<int, kag::N_ITEMS> first_buy{}; first_buy.fill(active_hours);
    std::array<int, 4> land_release{};
    std::array<std::array<double, kag::N_ITEMS>, 24> purchases{};
    for (const auto& e : p.market_plan) {
        if (e.market_op == kag::M_BUY_PRODUCT || e.market_op == kag::M_BUY_ANIMAL) {
            first_buy[e.item] = std::min(first_buy[e.item], int(e.hour) + 1);
            if (e.hour < active_hours - 1) purchases[e.hour + 1][e.item] += e.quantity;
        }
        if (e.market_op == kag::M_BUY_LAND) land_release[e.item] = e.hour + 1;
    }
    std::array<RouteTile, 100> route_tiles{}; int n = 0; unsigned all_inputs = 0;
    for (const auto& w : p.tile_work) {
        const auto& tile = p.start.managed_tiles[w.tile];
        auto& route = route_tiles[n++]; route.cell = tile.y * 10 + tile.x; route.work = w.actions.size();
        const int d = shed_distance(route.cell); bool productive = false;
        for (const auto& a : w.actions) {
            int item = -1;
            if (a.op == kag::OP_FEED) item = kag::WHEAT;
            if (a.op == kag::OP_FERTILIZE) item = kag::FERTILIZER;
            if (a.op == kag::OP_PLACE) item = a.arg;
            if (item >= 0) {
                route.inputs |= 1u << item; ++required_input[item];
                ++f[input_actions]; f[input_action_distance] += d;
            }
            if (a.output_item >= 0) {
                productive = true; ++f[output_actions]; f[output_action_distance] += d;
                output_by_distance[d][a.output_item] += a.output_quantity;
                produced[a.output_item] += a.output_quantity;
                max_yield[a.output_item] = std::max(max_yield[a.output_item], double(a.output_quantity));
            }
        }
        all_inputs |= route.inputs; f[productive_tiles] += productive;
        if (tile.state.kind == day_solver::ManagedTileKind::LOCKED)
            f[locked_work_pressure] += w.actions.size() * land_release[(tile.x >= 5) + 2 * (tile.y >= 5)];
    }
    f[distinct_inputs] = std::popcount(all_inputs);
    for (int i = 0; i < kag::N_ITEMS; ++i) {
        const double outside_need = std::max(0.0, required_input[i] - produced[i]);
        f[required_pickup_types] += outside_need > 0;
        if (outside_need > p.start.shed[i]) f[late_input_pressure] += outside_need * first_buy[i];
        f[output_kept_until_night] += std::max(0.0, produced[i] - required_input[i] - double(p.shed_availability[active_hours - 1][i]));
    }
    for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) if (i != j) {
        const int d = distance(route_tiles[i].cell, route_tiles[j].cell);
        const float work = std::min(route_tiles[i].work, route_tiles[j].work);
        if (d <= 1) f[neighbor_work_1] += work;
        if (d <= 2) f[neighbor_work_2] += work;
        if (d <= 3) f[neighbor_work_3] += work;
    }
    f[route_pack_open] = route_pack(route_tiles, n, false, active_hours - 1);
    f[route_pack_return] = route_pack(route_tiles, n, true, active_hours - 1);
    const int checkpoints[] = {0, 5, 11, 17, active_hours - 1};
    std::array<double, kag::N_ITEMS> available{};
    for (int i = 0; i < kag::N_ITEMS; ++i) available[i] = p.start.shed[i];
    for (int h = 0; h < active_hours; ++h) {
        double needed_ops = 0, deficit_total = 0; int radius = 0;
        for (int i = 0; i < kag::N_ITEMS; ++i) {
            available[i] += purchases[h][i];
            const double deficit = std::max(0.0, double(p.shed_availability[h][i]) - available[i]);
            deficit_total += deficit;
            if (!deficit) continue;
            if (max_yield[i] > 0) needed_ops += std::ceil(deficit / max_yield[i]);
            double reachable = 0, nearest = 0;
            for (int d = 0; d <= 8; ++d) {
                if (2 * d + 1 <= h) reachable += output_by_distance[d][i];
                if (nearest < deficit) { nearest += output_by_distance[d][i]; radius = std::max(radius, d); }
            }
            f[deadline_missing_quantity] = std::max(f[deadline_missing_quantity], float(std::max(0.0, deficit - reachable)));
        }
        const double work = needed_ops + (needed_ops > 0 ? 2 * radius + 1 : 0);
        f[deadline_required_actions_base + h] = work;
        f[deadline_pressure_max] = std::max(f[deadline_pressure_max], float(work / (h + 1)));
        f[deadline_required_ops_max] = std::max(f[deadline_required_ops_max], float(needed_ops));
        for (int k = 0; k < 5; ++k) if (h == checkpoints[k]) f[deadline_deficit_base + k] = deficit_total;
    }
}

// Pure physical input features. No worker_count, hire events, source IDs,
// schedules, solver outcomes or reference labels are read here.
inline Features extract_base(const day_solver::DayProblem& problem) {
    Features f{};
    std::array<int, 100> cells{}, task_counts{}, distance_sq{};
    std::array<bool, 100> active{};
    int n = 0, xmin = 10, xmax = -1, ymin = 10, ymax = -1;
    for (const auto& tile : problem.start.managed_tiles) {
        if (tile.state.kind == day_solver::ManagedTileKind::EMPTY) ++f[empty_tiles];
        if (tile.state.kind == day_solver::ManagedTileKind::LOCKED) ++f[locked_tiles];
        if (tile.state.crop >= 0) ++f[species_base + tile.state.crop];
        if (tile.state.animal >= 0) ++f[species_base + tile.state.animal];
    }
    for (const auto& work : problem.tile_work) {
        const auto& tile = problem.start.managed_tiles[work.tile];
        const int cell = tile.y * 10 + tile.x, d = shed_distance(cell);
        const int q = (tile.x >= 5) + 2 * (tile.y >= 5);
        cells[n] = cell; task_counts[n] = work.actions.size(); distance_sq[n] = d * d;
        active[cell] = true; ++n;
        xmin = std::min(xmin, int(tile.x)); xmax = std::max(xmax, int(tile.x));
        ymin = std::min(ymin, int(tile.y)); ymax = std::max(ymax, int(tile.y));
        f[tasks] += work.actions.size();
        f[max_tile_tasks] = std::max(f[max_tile_tasks], float(work.actions.size()));
        f[task_squared_sum] += work.actions.size() * work.actions.size();
        f[task_distance] += d * work.actions.size();
        f[distance_sum] += d; f[distance_max] = std::max(f[distance_max], float(d));
        f[quadrant_tasks_base + q] += work.actions.size(); ++f[quadrant_tiles_base + q];
        for (const auto& action : work.actions) {
            ++f[operations_base + action.op];
            int input = -1;
            if (action.op == kag::OP_FEED) input = kag::WHEAT;
            if (action.op == kag::OP_FERTILIZE) input = kag::FERTILIZER;
            if (action.op == kag::OP_PLACE) input = action.arg;
            if (input >= 0) { ++f[input_base + input]; f[input_distance] += d; }
            if (action.output_item >= 0) {
                f[output_base + action.output_item] += action.output_quantity;
                f[output_distance] += d * action.output_quantity;
            }
        }
    }
    f[active_tiles] = n; f[max_chain] = f[max_tile_tasks];
    if (n) {
        f[mean_tile_tasks] = f[tasks] / n;
        f[bbox_width] = xmax - xmin + 1; f[bbox_height] = ymax - ymin + 1;
    }
    double variance = 0;
    for (int i = 0; i < n; ++i) variance += task_counts[i] * distance_sq[i];
    if (f[tasks]) f[task_distance_std] = std::sqrt(std::max(0.0,
        variance / f[tasks] - std::pow(f[task_distance] / f[tasks], 2)));
    for (int i = 0; i < n; ++i) {
        int nearest = n > 1 ? 100 : 0;
        for (int j = 0; j < n; ++j) if (i != j) {
            const int d = distance(cells[i], cells[j]);
            nearest = std::min(nearest, d); f[pair_distance_mean] += d;
            if (d == 1) f[adjacent_pairs] += 0.5f;
        }
        f[nearest_distance_mean] += nearest;
    }
    if (n > 1) f[pair_distance_mean] /= n * (n - 1);
    if (n) f[nearest_distance_mean] /= n;
    // A super-root joins the four shed access squares with free edges. Any
    // collection of worker paths visiting all task tiles has at least this cost.
    std::array<int, 100> best{}; std::array<bool, 100> taken{};
    for (int i = 0; i < n; ++i) best[i] = shed_distance(cells[i]);
    for (int k = 0; k < n; ++k) {
        int selected = -1;
        for (int i = 0; i < n; ++i) if (!taken[i] && (selected < 0 || best[i] < best[selected])) selected = i;
        taken[selected] = true; f[rooted_mst] += best[selected];
        for (int i = 0; i < n; ++i) if (!taken[i]) best[i] = std::min(best[i], distance(cells[i], cells[selected]));
    }
    std::array<bool, 100> seen{}; std::array<int, 100> queue{};
    for (int i = 0; i < n; ++i) if (!seen[cells[i]]) {
        ++f[connected_components]; int begin = 0, end = 0;
        queue[end++] = cells[i]; seen[cells[i]] = true;
        while (begin < end) {
            const int cell = queue[begin++];
            for (int j = 0; j < n; ++j) if (!seen[cells[j]] && distance(cell, cells[j]) == 1) {
                seen[cells[j]] = true; queue[end++] = cells[j];
            }
        }
    }
    for (const auto& event : problem.market_plan) if (event.market_op != kag::M_HIRE) {
        ++f[purchase_events]; f[purchase_last_hour] = std::max(f[purchase_last_hour], float(event.hour));
        f[purchase_weighted_hour] += event.hour * double(event.quantity); f[purchase_units] += event.quantity;
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)
            f[bought_base + event.item] += event.quantity;
    }
    if (f[purchase_units]) f[purchase_weighted_hour] /= f[purchase_units];
    for (int i = 0; i < kag::N_ITEMS; ++i) {
        f[shed_base + i] = problem.start.shed[i];
        f[initial_shed_total] += problem.start.shed[i]; f[end_shed_total] += problem.end_shed[i];
    }
    for (int i = 0; i < kag::N_CROPS; ++i) f[seeds_base + i] = problem.start.seeds[i];
    f[first_withdrawal_hour] = 24;
    constexpr int checkpoints[] = {0, 5, 11, 17, 23};
    for (int h = 0; h < 24; ++h) for (int i = 0; i < kag::N_ITEMS; ++i) {
        const auto amount = problem.shed_availability[h][i] - (h ? problem.shed_availability[h - 1][i] : 0);
        if (amount) {
            ++f[withdrawal_events]; f[withdrawal_total] += amount;
            f[first_withdrawal_hour] = std::min(f[first_withdrawal_hour], float(h));
            f[last_withdrawal_hour] = h; f[weighted_withdrawal_hour] += h * double(amount);
        }
        for (int k = 0; k < 5; ++k) if (h == checkpoints[k])
            f[deadlines_base + k * kag::N_ITEMS + i] = problem.shed_availability[h][i];
        if (h == 5) f[early_withdrawal_deficit] += std::max<int64_t>(0, problem.shed_availability[h][i] - problem.start.shed[i]);
    }
    if (f[withdrawal_total]) f[weighted_withdrawal_hour] /= f[withdrawal_total];
    f[route_work_proxy] = f[tasks] + 2 * f[rooted_mst] + f[input_distance] / 12 + f[output_distance] / 12;
    f[task_motion_bound] = f[tasks] + f[rooted_mst];
    return f;
}

inline Features extract(const day_solver::DayProblem& problem, int active_hours = 24) {
    if (active_hours != 23 && active_hours != 24) throw std::runtime_error("unsupported active horizon");
    if (active_hours == 23) {
        if (problem.shed_availability[23] != problem.shed_availability[22]) throw std::runtime_error("terminal withdrawal in virtual phase");
        for (const auto& event : problem.market_plan)
            if (event.hour == 23) throw std::runtime_error("terminal purchase in virtual phase");
    }
    auto f = extract_base(problem);
    if (f[first_withdrawal_hour] == 24) f[first_withdrawal_hour] = active_hours;
    add_pressure_features(problem, f, active_hours);
    return f;
}

inline std::vector<std::string> feature_names() {
    std::vector<std::string> result = {"tasks", "active_tiles", "max_tile_tasks", "mean_tile_tasks", "task_squared_sum",
        "task_distance", "input_distance", "output_distance", "distance_sum", "distance_max", "bbox_width", "bbox_height",
        "pair_distance_mean", "nearest_distance_mean", "rooted_mst", "adjacent_pairs", "connected_components",
        "task_distance_std", "empty_tiles", "locked_tiles", "purchase_events", "purchase_last_hour", "purchase_weighted_hour",
        "purchase_units", "initial_shed_total", "end_shed_total", "withdrawal_total", "withdrawal_events",
        "first_withdrawal_hour", "last_withdrawal_hour", "weighted_withdrawal_hour", "early_withdrawal_deficit",
        "max_chain", "route_work_proxy", "task_motion_bound"};
    auto add = [&](const std::string& prefix, int length) { for (int i = 0; i < length; ++i) result.push_back(prefix + std::to_string(i)); };
    add("op_", 18); add("input_", kag::N_ITEMS); add("output_", kag::N_ITEMS); add("shed_", kag::N_ITEMS);
    add("bought_", kag::N_ITEMS); add("seed_", kag::N_CROPS); add("withdrawal_checkpoint_", 5 * kag::N_ITEMS);
    add("quadrant_tasks_", 4); add("quadrant_tiles_", 4); add("species_", kag::N_ITEMS);
    for (const auto& name : {"productive_tiles", "output_actions", "output_action_distance", "input_actions", "input_action_distance",
         "distinct_inputs", "required_pickup_types", "output_kept_until_night", "deadline_missing_quantity", "deadline_pressure_max",
         "deadline_required_ops_max", "late_input_pressure", "locked_work_pressure", "neighbor_work_1", "neighbor_work_2", "neighbor_work_3",
         "route_pack_open", "route_pack_return"}) result.emplace_back(name);
    add("deadline_required_actions_", 24); add("deadline_deficit_", 5);
    if (result.size() != count) std::abort();
    return result;
}
}
