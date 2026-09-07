#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace day_semantic {
using Route = std::vector<int>;
using Routes = std::vector<Route>;
using Candidates = std::vector<Routes>;
using Count = std::int64_t;

inline void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

enum class Action { other, place, feed, care };
struct Task {
    int tile, pattern, x, y, input, output, predecessor;
    Action action = Action::other;
};
struct Group { std::set<int> sources, targets; };
struct Violation { int predecessor, successor; };
struct Deficit { int item, deadline; Count quantity; };
struct Delivery {
    int item, route;
    Count quantity;
    std::map<int, Count> delivered_by;
};
struct SourceMove { int item, successor, old_route, new_route; };
struct Assignment { int task, worker, rank, type; };
struct Hint { std::vector<Assignment> assignments; std::vector<Route> type_workers; };
using OwnerKey = std::vector<std::pair<Route, Route>>;

inline Routes read_routes(const Hint& hint) {
    std::map<int, std::vector<std::pair<int, int>>> grouped;
    for (auto value : hint.assignments) grouped[value.worker].emplace_back(value.rank, value.task);
    Routes routes;
    for (auto& [worker, entries] : grouped) {
        std::sort(entries.begin(), entries.end());
        Route route;
        for (auto [rank, task] : entries) route.push_back(task);
        routes.push_back(std::move(route));
    }
    return routes;
}

inline std::map<std::pair<int, int>, Route> grouped_tasks(const Hint& hint) {
    std::map<std::pair<int, int>, Route> result;
    for (auto value : hint.assignments) result[{value.type, value.worker}].push_back(value.task);
    return result;
}

inline OwnerKey owner_key(const Hint& hint, const std::set<int>& free_routes = {}) {
    OwnerKey result;
    int route = 0;
    for (auto [group, tasks] : grouped_tasks(hint)) {
        if (!free_routes.contains(route)) {
            auto workers = hint.type_workers.at(group.first);
            std::sort(workers.begin(), workers.end());
            std::sort(tasks.begin(), tasks.end());
            result.emplace_back(std::move(workers), std::move(tasks));
        }
        ++route;
    }
    std::sort(result.begin(), result.end());
    return result;
}

inline Hint write_hint(const Routes& routes, const Hint& source) {
    std::set<int> workers;
    std::map<int, int> types;
    for (auto value : source.assignments) workers.insert(value.worker);
    for (int type = 0; type < int(source.type_workers.size()); ++type)
        for (int worker : source.type_workers[type]) types[worker] = type;
    require(routes.size() == workers.size(), "repair changed the number of source routes");
    Hint result{{}, source.type_workers};
    int index = 0;
    for (int worker : workers) {
        int rank = 0;
        for (int task : routes[index++]) result.assignments.push_back({task, worker, rank++, types[worker]});
    }
    return result;
}

inline int lookup(const std::map<int, int>& values, int key, int missing) {
    auto found = values.find(key);
    return found == values.end() ? missing : found->second;
}

inline Candidates unique(Candidates candidates, bool reject_empty = false) {
    std::set<Routes> seen;
    Candidates result;
    for (auto& routes : candidates) {
        if (reject_empty && std::any_of(routes.begin(), routes.end(), [](const auto& r) { return r.empty(); })) continue;
        if (seen.insert(routes).second) result.push_back(std::move(routes));
    }
    return result;
}

struct Context {
    std::vector<Task> tasks;
    std::map<int, Group> groups;
    std::map<int, int> deadlines, late_inputs;

    Count length(const Route& route) const {
        if (route.empty()) return 0;
        const auto& first = tasks.at(route[0]);
        Count travel = std::numeric_limits<Count>::max();
        for (auto point : std::array<std::array<int, 2>, 4>{{{4, 4}, {5, 4}, {4, 5}, {5, 5}}})
            travel = std::min(travel, std::abs(Count(first.x) - point[0]) + std::abs(Count(first.y) - point[1]));
        std::set<int> inputs;
        for (std::size_t i = 0; i < route.size(); ++i) {
            const auto& task = tasks.at(route[i]);
            if (task.input >= 0) inputs.insert(task.input);
            if (i) {
                const auto& previous = tasks.at(route[i - 1]);
                travel += std::abs(Count(task.x) - previous.x) + std::abs(Count(task.y) - previous.y);
            }
        }
        return Count(route.size()) + travel + Count(inputs.size());
    }

    Count scarce_deficit(const Routes& routes) const {
        Count result = 0;
        for (const auto& [item, group] : groups)
            for (const auto& route : routes) {
                Count delta = 0;
                for (int task : route) delta += int(group.targets.contains(task)) - int(group.sources.contains(task));
                result += std::max(Count(0), delta);
            }
        return result;
    }

    Routes restore_tile_order(Routes routes) const {
        for (auto& route : routes) {
            std::map<int, std::vector<int>> slots;
            for (int i = 0; i < int(route.size()); ++i) slots[tasks.at(route[i]).tile].push_back(i);
            for (const auto& [tile, positions] : slots) {
                Route ordered;
                for (int i : positions) ordered.push_back(route[i]);
                std::sort(ordered.begin(), ordered.end());
                for (std::size_t i = 0; i < positions.size(); ++i) route[positions[i]] = ordered[i];
            }
        }
        return routes;
    }

    int late_input_route(const Hint& hint) const {
        std::tuple<int, std::size_t, int> best{};
        int index = 0, result = -1;
        for (const auto& [key, ids] : grouped_tasks(hint)) {
            int ready = 0;
            for (int task : ids) ready = std::max(ready, lookup(late_inputs, task, 0));
            const auto score = std::tuple{ready, ids.size(), -index};
            if (ready && (result < 0 || score > best)) { best = score; result = index; }
            ++index;
        }
        return result;
    }

    static int find_route(const Routes& routes, int task) {
        for (int i = 0; i < int(routes.size()); ++i)
            if (std::find(routes[i].begin(), routes[i].end(), task) != routes[i].end()) return i;
        throw std::runtime_error("task missing from proposal");
    }

    static void remove(Route& route, int task) {
        auto found = std::find(route.begin(), route.end(), task);
        require(found != route.end(), "task missing from source route");
        route.erase(found);
    }

    static int move(Routes& routes, int task, int target, int anchor, bool before) {
        const int source = find_route(routes, task);
        remove(routes[source], task);
        auto& destination = routes.at(target);
        auto found = std::find(destination.begin(), destination.end(), anchor);
        require(found != destination.end(), "anchor missing from target route");
        destination.insert(found + int(!before), task);
        return source;
    }

    std::vector<int> insertion_positions(const Route& base, const Route& block, int first = 0) const {
        std::vector<std::pair<Count, int>> scores;
        for (int index = first; index <= int(base.size()); ++index) {
            Route candidate = base;
            candidate.insert(candidate.begin() + index, block.begin(), block.end());
            scores.emplace_back(length(candidate), index);
        }
        std::sort(scores.begin(), scores.end());
        std::vector<int> result;
        for (auto [score, index] : scores) result.push_back(index);
        return result;
    }

    Routes insert_task(Routes routes, int task, int target, int after) const {
        remove(routes[find_route(routes, task)], task);
        auto& destination = routes.at(target);
        auto found = std::find(destination.begin(), destination.end(), after);
        const int first = found == destination.end() ? 0 : int(found - destination.begin()) + 1;
        const int position = insertion_positions(destination, {task}, first).at(0);
        destination.insert(destination.begin() + position, task);
        return routes;
    }

    Candidates insert_block(Routes routes, const Route& block, int source, int target) const {
        for (int task : block) remove(routes.at(source), task);
        const auto positions = insertion_positions(routes.at(target), block);
        Candidates result;
        for (std::size_t index = 0; index < std::min(std::size_t(2), positions.size()); ++index) {
            Routes candidate = routes;
            auto& destination = candidate[target];
            destination.insert(destination.begin() + positions[index], block.begin(), block.end());
            result.push_back(std::move(candidate));
        }
        return result;
    }

    Candidates balanced_variants(const Routes& routes, const std::vector<SourceMove>& moves) const {
        Candidates variants{routes};
        for (auto move : moves) {
            const auto found = groups.find(move.item);
            if (found == groups.end()) continue;
            const auto& group = found->second;
            Candidates next;
            for (const auto& candidate : variants) {
                int sources = 0;
                Route targets;
                for (int task : candidate[move.old_route]) {
                    sources += group.sources.contains(task);
                    if (group.targets.contains(task)) targets.push_back(task);
                }
                if (sources >= int(targets.size())) { next.push_back(candidate); continue; }
                const int limit = lookup(deadlines, move.successor, 23);
                int anchor = move.successor;
                for (int task : candidate[move.new_route]) if (lookup(deadlines, task, 24) <= limit) anchor = task;
                for (int task : targets) next.push_back(insert_task(candidate, task, move.new_route, anchor));
            }
            variants = std::move(next);
            if (variants.empty()) break;
        }
        variants.insert(variants.begin(), routes); // A shared shed transfer may supply the direct move.
        return unique(std::move(variants));
    }

    Candidates semantic_neighbors(const Routes& routes, const std::vector<Violation>& violations, int max_arcs) const {
        require(max_arcs >= 0, "negative maximum arcs");
        const int count = std::min(int(violations.size()), max_arcs);
        Candidates result;
        std::vector<int> selected, directions;
        auto evaluate = [&] {
            Routes candidate = routes;
            std::vector<SourceMove> moves;
            for (std::size_t i = 0; i < selected.size(); ++i) {
                const auto arc = violations[selected[i]];
                const int source = find_route(candidate, arc.predecessor), target = find_route(candidate, arc.successor);
                if (source == target) continue;
                if (directions[i] == 0) {
                    move(candidate, arc.predecessor, target, arc.successor, true);
                    const int item = tasks.at(arc.predecessor).output;
                    if (item >= 0) moves.push_back({item, arc.successor, source, target});
                } else move(candidate, arc.successor, source, arc.predecessor, false);
                if (std::any_of(candidate.begin(), candidate.end(), [](const auto& r) { return r.empty(); })) return;
            }
            auto balanced = balanced_variants(candidate, moves);
            result.insert(result.end(), std::make_move_iterator(balanced.begin()), std::make_move_iterator(balanced.end()));
        };
        std::function<void(int)> choose_directions = [&](int index) {
            if (index == int(selected.size())) { evaluate(); return; }
            for (int direction : {0, 1}) { directions.push_back(direction); choose_directions(index + 1); directions.pop_back(); }
        };
        std::function<void(int, int)> choose = [&](int start, int left) {
            if (!left) { choose_directions(0); return; }
            for (int i = start; i <= count - left; ++i) { selected.push_back(i); choose(i + 1, left - 1); selected.pop_back(); }
        };
        for (int size = 1; size <= count; ++size) choose(0, size);
        return unique(std::move(result));
    }

    std::vector<int> target_routes(const Routes& routes, int source) const {
        std::vector<int> targets;
        for (int i = 0; i < int(routes.size()); ++i) if (i != source) targets.push_back(i);
        std::sort(targets.begin(), targets.end(), [&](int a, int b) { return std::pair{length(routes[a]), a} < std::pair{length(routes[b]), b}; });
        return targets;
    }

    Candidates availability_neighbors(const Routes& routes, const std::vector<Deficit>& deficits, const std::vector<Delivery>& profiles) const {
        Candidates result;
        for (auto deficit : deficits) {
            std::set<int> blocked;
            for (const auto& profile : profiles) {
                auto found = profile.delivered_by.find(deficit.deadline);
                const Count delivered = found == profile.delivered_by.end() ? 0 : found->second;
                if (profile.item == deficit.item && delivered < profile.quantity) blocked.insert(profile.route);
            }
            for (int source : blocked) {
                const auto& route = routes.at(source);
                auto targets = target_routes(routes, source);
                targets.resize(std::min(std::size_t(6), targets.size()));
                for (std::size_t begin = 0; begin < route.size();) {
                    std::size_t end = begin + 1;
                    while (end < route.size() && tasks.at(route[end]).pattern == tasks.at(route[begin]).pattern) ++end;
                    const Route block(route.begin() + begin, route.begin() + end);
                    for (int target : targets) {
                        auto candidates = insert_block(routes, block, source, target);
                        result.insert(result.end(), std::make_move_iterator(candidates.begin()), std::make_move_iterator(candidates.end()));
                    }
                    begin = end;
                }
            }
        }
        return unique(std::move(result), true);
    }

    std::pair<Count, Count> lengths(const Routes& routes) const {
        Count longest = 0, total = 0;
        for (const auto& route : routes) { const auto value = length(route); longest = std::max(longest, value); total += value; }
        return {longest, total};
    }

    Candidates late_placement_neighbors(const Routes& routes) const {
        std::map<int, Route> successors;
        for (int i = 0; i < int(tasks.size()); ++i) successors[tasks[i].predecessor].push_back(i);
        Candidates result;
        for (auto [target, ready] : late_inputs) {
            const auto& task = tasks.at(target);
            if (task.action != Action::place) continue;
            Route block{target};
            int current = target;
            while (true) {
                Route following;
                for (int successor : successors[current]) {
                    const auto& next = tasks[successor];
                    if (next.x == task.x && next.y == task.y && (next.action == Action::feed || next.action == Action::care)) following.push_back(successor);
                }
                if (following.size() != 1) break;
                current = following[0];
                block.push_back(current);
                require(block.size() <= tasks.size(), "cyclic placement successors");
            }
            const int source = find_route(routes, target);
            if (task.predecessor < 0 || std::find(routes[source].begin(), routes[source].end(), task.predecessor) == routes[source].end()) continue;
            for (int destination : target_routes(routes, source)) {
                auto candidates = insert_block(routes, block, source, destination);
                result.insert(result.end(), std::make_move_iterator(candidates.begin()), std::make_move_iterator(candidates.end()));
            }
        }
        result = unique(std::move(result), true);
        std::sort(result.begin(), result.end(), [&](const auto& a, const auto& b) { return std::tuple{lengths(a), a} < std::tuple{lengths(b), b}; });
        return result;
    }

    Candidates rank(const Candidates& candidates, std::set<Routes>& seen) const {
        using Entry = std::tuple<Count, Count, Count, Routes>;
        std::vector<Entry> ranked;
        for (auto candidate : candidates) {
            candidate = restore_tile_order(std::move(candidate));
            if (!seen.insert(candidate).second) continue;
            const auto [longest, total] = lengths(candidate);
            ranked.emplace_back(scarce_deficit(candidate), longest, total, std::move(candidate));
        }
        std::sort(ranked.begin(), ranked.end());
        Candidates result;
        for (auto& entry : ranked) result.push_back(std::move(std::get<3>(entry)));
        return result;
    }
};
} // namespace day_semantic
