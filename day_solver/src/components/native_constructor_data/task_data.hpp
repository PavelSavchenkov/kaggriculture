#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <boost/json.hpp>
#include "problem_json.hpp"
#include "problem_validation.hpp"
#include "tile_graph.hpp"

namespace day_constructor {
namespace ds = day_solver;
namespace json = boost::json;
namespace fs = std::filesystem;
using Count = ds::InventoryCount;
using Point = std::array<int, 2>;
using Key = std::pair<int, int>;
constexpr int hours = 24, items = kag::N_ITEMS, crops = kag::N_CROPS;
constexpr std::array<Point, 4> shed{{{4, 4}, {5, 4}, {4, 5}, {5, 5}}};
inline void require(bool condition, const std::string& reason) {
    if (!condition) throw std::runtime_error(reason);
}
inline int distance(Point a, Point b) { return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]); }
inline int tail(Point point) {
    int best = 20;
    for (auto access : shed) best = std::min(best, distance(point, access));
    return best;
}
inline Point nearest_shed(Point point) {
    return *std::min_element(shed.begin(), shed.end(), [&](Point a, Point b) { return distance(a, point) < distance(b, point); });
}
struct Task {
    int id, pattern, tile, predecessor, input, crop, output, op, arg;
    Count quantity, action_quantity;
    Point point;
};
struct TaskData {
    const ds::DayProblem& problem;
    std::vector<Task> tasks;
    std::vector<int> hires, early, late;
    std::map<Key, std::vector<int>> deliveries;
    std::map<int, int> fixed_deadline;
    Count bought(int item, int hour) const {
        Count quantity = 0;
        for (const auto& event : problem.market_plan)
            if ((event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL) &&
                event.item == item && event.hour < hour) quantity += event.quantity;
        return quantity;
    }

    std::vector<int> delivery_earliest() const {
        std::vector<int> result(tasks.size(), hours);
        std::vector<std::vector<int>> tile_tasks(problem.start.managed_tiles.size());
        for (const auto& task : tasks) tile_tasks[task.tile].push_back(task.id);
        for (auto graph : ds::build_tile_graphs(problem)) {
            const auto& tile = problem.start.managed_tiles[graph.tile];
            const Point point{tile.x, tile.y};
            std::array<int, hours> reachable{};
            for (int hour = 0; hour < hours; ++hour) {
                reachable[hour] = int(distance(shed[0], point) <= hour);
                for (int hire : hires) reachable[hour] += hire + 1 + tail(point) <= hour;
            }
            std::erase_if(graph.arcs, [&](const auto& arc) {
                const auto& node = graph.nodes[arc.from];
                return arc.prefix - node.prefix > reachable[node.hour];
            });
            std::vector<std::vector<int>> forward(graph.nodes.size()), reverse(graph.nodes.size());
            for (const auto& arc : graph.arcs) {
                forward[arc.from].push_back(arc.to);
                reverse[arc.to].push_back(arc.from);
            }
            auto visit = [&](std::vector<int> queue, const auto& edges) {
                std::vector<bool> seen(graph.nodes.size());
                for (int node : queue) seen[node] = true;
                for (std::size_t i = 0; i < queue.size(); ++i)
                    for (int next : edges[queue[i]]) if (!seen[next]) { seen[next] = true; queue.push_back(next); }
                return seen;
            };
            const auto forward_seen = visit({0}, forward), reverse_seen = visit(graph.terminals, reverse);
            require(int(tile_tasks[graph.tile].size()) == graph.task_count, "tile/task count mismatch");
            for (const auto& arc : graph.arcs) {
                if (!forward_seen[arc.from] || !reverse_seen[arc.from] || !forward_seen[arc.to] || !reverse_seen[arc.to]) continue;
                const auto& node = graph.nodes[arc.from];
                for (int local = node.prefix; local < arc.prefix; ++local) {
                    const int task = tile_tasks[graph.tile][local];
                    result[task] = std::min(result[task], node.hour + 1 + tail(point));
                }
            }
        }
        return result;
    }

    int delivery_estimate(int id) const {
        std::set<int> pickups;
        int length = 0;
        for (int task = id; task >= 0; task = tasks[task].predecessor) {
            ++length;
            if (tasks[task].input >= 0) pickups.insert(tasks[task].input);
        }
        return 2 * tail(tasks[id].point) + length + pickups.size();
    }

    void select_deliveries() {
        bool needed = false;
        for (int hour = 0; hour < hours; ++hour)
            for (int item = 0; item < items; ++item)
                needed |= problem.shed_availability[hour][item] > problem.start.shed[item] + bought(item, hour);
        if (!needed) return;
        const auto earliest = delivery_earliest();
        std::array<std::vector<int>, items> candidates;
        for (const auto& task : tasks) if (task.output >= 0) candidates[task.output].push_back(task.id);
        for (auto& ids : candidates)
            std::sort(ids.begin(), ids.end(), [&](int a, int b) {
                return std::tuple(delivery_estimate(a), tail(tasks[a].point), -tasks[a].quantity, a) <
                       std::tuple(delivery_estimate(b), tail(tasks[b].point), -tasks[b].quantity, b);
            });
        std::array<Count, items> selected{};
        std::vector<bool> used(tasks.size());
        for (int hour = 0; hour < hours; ++hour)
            for (int item = 0; item < items; ++item) {
                const Count deficit = std::max(Count(0), problem.shed_availability[hour][item] - problem.start.shed[item] - bought(item, hour));
                if (selected[item] >= deficit) continue;
                for (int task : candidates[item]) {
                    if (used[task] || earliest[task] > hour) continue;
                    used[task] = true;
                    selected[item] += tasks[task].quantity;
                    deliveries[{item, hour}].push_back(task);
                    fixed_deadline[task] = hour;
                    if (selected[item] >= deficit) break;
                }
                require(selected[item] >= deficit, "not enough output by availability deadline");
            }
    }

    void task_windows() {
        early.assign(tasks.size(), 0);
        late.assign(tasks.size(), hours - 1);
        constexpr std::array<int, crops> loss_age{5, 4, 12, 17, 13};
        for (const auto& task : tasks) {
            const auto& state = problem.start.managed_tiles[task.tile].state;
            if (task.op != kag::OP_HARVEST || state.kind != ds::ManagedTileKind::CROP || state.crop < 0 || state.age_days < loss_age[state.crop]) continue;
            const Count losses = state.stored_units - task.quantity;
            require(losses >= 0 && losses <= 12, "invalid decaying crop harvest");
            early[task.id] = losses ? 2 * losses - 1 : 0;
            late[task.id] = std::min(Count(hours - 1), 2 * losses);
        }
        std::array<int, crops> first;
        first.fill(hours);
        for (int crop = 0; crop < crops; ++crop) {
            int plants = 0;
            for (const auto& task : tasks) plants += task.crop == crop;
            if (!plants) continue;
            Count supply = problem.start.seeds[crop];
            if (supply > 0) first[crop] = 0;
            for (const auto& event : problem.market_plan)
                if (event.market_op == kag::M_BUY_SEED && event.item == crop) {
                    supply += event.quantity;
                    if (event.quantity > 0) first[crop] = std::min(first[crop], int(event.hour) + 1);
                }
            require(supply >= plants, "insufficient fixed seeds");
        }
        for (const auto& task : tasks) {
            if (task.crop >= 0) early[task.id] = first[task.crop];
            else if (task.predecessor >= 0) early[task.id] = early[task.predecessor];
        }
        for (const auto& task : tasks) {
            if (problem.start.managed_tiles[task.tile].state.kind != ds::ManagedTileKind::LOCKED) continue;
            const int quadrant = int(task.point[0] >= 5) + 2 * int(task.point[1] >= 5);
            int release = hours;
            for (const auto& event : problem.market_plan)
                if (event.market_op == kag::M_BUY_LAND && event.item == quadrant) release = event.hour + 1;
            early[task.id] = std::max(early[task.id], release);
        }
        for (const auto& task : tasks) if (task.input >= 0)
            for (const auto& successor : tasks)
                if (successor.tile == task.tile && successor.id >= task.id)
                    early[successor.id] = std::max(early[successor.id], 1 + tail(task.point));
        for (const auto& task : tasks) require(early[task.id] <= late[task.id], "inconsistent task windows");
    }

    explicit TaskData(const ds::DayProblem& supplied) : problem(supplied) {
        require(problem.format_version == 3 && ds::validate_problem(problem).empty(), "valid v3 input required");
        for (const auto& event : problem.market_plan) if (event.market_op == kag::M_HIRE) hires.push_back(event.hour);
        std::sort(hires.begin(), hires.end());
        require(int(hires.size()) + 1 == problem.worker_count, "fixed hires disagree with worker count");
        for (int pattern = 0; pattern < int(problem.tile_work.size()); ++pattern) {
            const auto& work = problem.tile_work[pattern];
            int previous = -1;
            const auto& tile = problem.start.managed_tiles[work.tile];
            for (const auto& action : work.actions) {
                const int input = action.op == kag::OP_FEED ? kag::WHEAT : action.op == kag::OP_FERTILIZE ? kag::FERTILIZER : action.op == kag::OP_PLACE ? action.arg : -1;
                tasks.push_back({int(tasks.size()), pattern, work.tile, previous, input, action.op == kag::OP_PLANT ? action.arg : -1,
                                 action.output_item, action.op, action.arg, action.output_quantity, action.quantity, {tile.x, tile.y}});
                previous = tasks.back().id;
            }
        }
        select_deliveries();
        task_windows();
        // Constructor windows include return deadlines; the coarse checker
        // deliberately omits these hard assignments in its relaxation.
        for (const auto& [id, deadline] : fixed_deadline) {
            const int last = deadline - tail(tasks[id].point) - 1;
            for (int task = id; task >= 0; task = tasks[task].predecessor)
                late[task] = std::min(late[task], last);
        }
        for (const auto& task : tasks) require(early[task.id] <= late[task.id], "inconsistent constructor task windows");
    }
};
}  // namespace day_constructor
