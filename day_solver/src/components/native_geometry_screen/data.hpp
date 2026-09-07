#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "problem_validation.hpp"
#include "tile_graph.hpp"

#include "screen.hpp"

namespace day_native::geometry_screen {
namespace ds = day_solver;
using Count = ds::InventoryCount;
using Point = std::array<int, 2>;
using Key = std::pair<int, int>;
constexpr int hours = 24, items = kag::N_ITEMS, crops = kag::N_CROPS;
constexpr std::array<Point, 4> shed{{{4, 4}, {5, 4}, {4, 5}, {5, 5}}};

void require(bool condition, const std::string& reason) {
    if (!condition) throw std::runtime_error(reason);
}
int distance(Point a, Point b) { return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]); }
int tail(Point point) {
    int best = 20;
    for (auto access : shed) best = std::min(best, distance(point, access));
    return best;
}
std::string name(const std::string& prefix, std::initializer_list<int> indices) {
    std::string result = prefix + "[";
    for (int index : indices) {
        if (result.back() != '[') result += ',';
        result += std::to_string(index);
    }
    return result + ']';
}
std::string point_name(Point point) { return "(" + std::to_string(point[0]) + ", " + std::to_string(point[1]) + ")"; }
struct Assignment { int task, worker, hour; };

struct ScreenData {
    ds::DayProblem problem;
    ScreenOptions options;
    std::vector<Task> tasks;
    std::vector<int> hires, early, late, owner, route_ids, worker_group;
    std::vector<Profile> workers, profiles;
    std::vector<std::vector<int>> groups, source_groups, route_tasks;
    std::vector<Assignment> assignments;
    std::vector<std::vector<Key>> hinted;
    std::map<int, int> source_type;
    std::vector<std::pair<Key, Count>> requirements;
    std::map<Key, std::vector<int>> deliveries;
    std::map<int, int> fixed_deadline;
    std::map<Key, Count> net;
    std::set<Key> surplus;
    std::set<int> delivery_tasks;
    std::vector<int> deadlines;

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
        if (requirements.empty()) return;
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

    ScreenData(ds::DayProblem supplied, const InternalHint& source, ScreenOptions supplied_options)
        : problem(std::move(supplied)), options(std::move(supplied_options)) {
        require(problem.format_version == 3 && ds::validate_problem(problem).empty(), "valid v3 input required");
        for (const auto& event : problem.market_plan) if (event.market_op == kag::M_HIRE) hires.push_back(event.hour);
        std::sort(hires.begin(), hires.end());
        require(int(hires.size()) + 1 == problem.worker_count, "fixed hires disagree with worker count");
        for (const auto& work : problem.tile_work) {
            int previous = -1;
            const auto& tile = problem.start.managed_tiles[work.tile];
            for (const auto& action : work.actions) {
                const int input = action.op == kag::OP_FEED ? kag::WHEAT : action.op == kag::OP_FERTILIZE ? kag::FERTILIZER : action.op == kag::OP_PLACE ? action.arg : -1;
                tasks.push_back({int(tasks.size()), work.tile, previous, input, action.op == kag::OP_PLANT ? action.arg : -1,
                                 action.output_item, action.op, action.output_quantity, {tile.x, tile.y}});
                previous = tasks.back().id;
            }
        }
        if (!options.ignore_availability) {
            std::array<Count, items> previous{};
            for (int hour = 0; hour < hours; ++hour)
                for (int item = 0; item < items; ++item) {
                    const Count required = std::max(previous[item], problem.shed_availability[hour][item] - problem.start.shed[item] - bought(item, hour));
                    if (required > previous[item]) { requirements.push_back({{item, hour}, required}); previous[item] = required; }
                }
        }
        select_deliveries();
        task_windows();
        std::map<int, int> raw_owner;
        for (const auto& assignment : source.assignments) {
            const int task = assignment.task, worker = assignment.worker, hour = required(assignment.hour, "hour");
            require(task >= 0 && task < int(tasks.size()) && !raw_owner.contains(task) && worker >= 0, "invalid owner assignment");
            raw_owner[task] = worker;
            assignments.push_back({task, worker, hour});
        }
        require(raw_owner.size() == tasks.size(), "owner hint must cover all tasks");
        std::set<int> route_set;
        for (const auto& [task, worker] : raw_owner) route_set.insert(worker);
        route_ids.assign(route_set.begin(), route_set.end());
        require(route_ids.size() <= std::size_t(problem.worker_count), "more routes than workers");
        owner.resize(tasks.size()); route_tasks.resize(route_ids.size()); hinted.resize(route_ids.size());
        for (const auto& assignment : assignments) {
            const int route = std::lower_bound(route_ids.begin(), route_ids.end(), assignment.worker) - route_ids.begin();
            owner[assignment.task] = route;
            route_tasks[route].push_back(assignment.task);
            hinted[route].push_back({assignment.hour, assignment.task});
        }
        for (const auto& task : tasks) {
            if (task.input >= 0) --net[{owner[task.id], task.input}];
            if (task.output >= 0) net[{owner[task.id], task.output}] += task.quantity;
        }
        std::array<Count, items> positive{}, needed{};
        std::array<bool, items> consumed{}, retained{};
        for (const auto& task : tasks) if (task.input >= 0) consumed[task.input] = true;
        for (const auto& [key, quantity] : net) positive[key.second] += std::max(Count(0), quantity);
        for (const auto& [key, quantity] : requirements) needed[key.first] = std::max(needed[key.first], quantity);
        for (int item = 0; item < items; ++item)
            retained[item] = positive[item] > problem.end_shed[item] && (consumed[item] || needed[item] < positive[item] - problem.end_shed[item]);
        std::erase_if(net, [&](const auto& entry) { return !retained[entry.first.second]; });
        if (!options.ignore_availability)
            for (const auto& [key, quantity] : net) if (quantity > 0) surplus.insert(key);
        std::set<int> deadline_set;
        if (options.fixed_availability) for (const auto& [key, ids] : deliveries) deadline_set.insert(key.second);
        else for (const auto& [key, quantity] : requirements) deadline_set.insert(key.second);
        if (!surplus.empty()) deadline_set.insert(hours - 1);
        deadlines.assign(deadline_set.begin(), deadline_set.end());
        for (const auto& task : tasks)
            if (std::any_of(requirements.begin(), requirements.end(), [&](const auto& r) { return r.first.first == task.output; }) || surplus.contains({owner[task.id], task.output}))
                delivery_tasks.insert(task.id);
        std::vector<Point> starts{shed[0]};
        std::array<int, 4> occupancy{1, 0, 0, 0}, start_counts{1, 0, 0, 0};
        workers.push_back({{shed[0]}, 0});
        int first_hire = 0;
        for (int i = 0; i < int(hires.size()); ++i) {
            const auto& counts = hires[i] == 0 ? occupancy : start_counts;
            const int access = std::min_element(counts.begin(), counts.end()) - counts.begin();
            starts.push_back(shed[access]); ++start_counts[access];
            if (hires[i] == 0) ++occupancy[access];
            if (!i || hires[i] != hires[i - 1]) first_hire = i;
            std::vector<Point> choices;
            if (options.dynamic) {
                for (int p = 0; p < 4; ++p) if (((p - (i - first_hire)) % 4 + 4) % 4 <= first_hire + 1) choices.push_back(shed[p]);
            } else choices.push_back(starts.back());
            workers.push_back({choices, hires[i] + 1});
        }
        for (int worker = 0; worker < int(workers.size()); ++worker) {
            const auto found = std::find(profiles.begin(), profiles.end(), workers[worker]);
            const int group = found - profiles.begin();
            if (found == profiles.end()) { profiles.push_back(workers[worker]); groups.emplace_back(); }
            groups[group].push_back(worker); worker_group.push_back(group);
        }
        if (source.type_workers) source_groups = *source.type_workers;
        if (options.fixed_profile) {
            std::vector<int> all, expected(problem.worker_count);
            for (int type = 0; type < int(source_groups.size()); ++type)
                for (int worker : source_groups[type]) { all.push_back(worker); source_type[worker] = type; }
            std::sort(all.begin(), all.end()); std::iota(expected.begin(), expected.end(), 0);
            require(all == expected, "source worker groups must partition workers");
            for (int route : route_ids) require(source_type.contains(route), "route not in worker groups");
        }
    }
};

} // namespace day_native::geometry_screen
