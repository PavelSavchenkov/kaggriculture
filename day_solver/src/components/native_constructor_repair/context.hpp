#pragma once
#include "bundles.hpp"
#include "kernels.hpp"

namespace day_constructor {
using BundleRoute = std::vector<int>;
using BundlePlan = std::vector<BundleRoute>;
using CheapScore = std::array<Count, 9>;
using NativeHandle = std::unique_ptr<void, void (*)(void*)>;

template <class T> const T* buffer(const std::vector<T>& values) {
    static const T empty = 0;
    return values.empty() ? &empty : values.data();
}

struct StrongScore {
    int unmatched, impossible;
    std::vector<int> impossible_routes;
};
struct Propagation {
    bool cyclic;
    int lateness;
    std::vector<int> lower;
};
struct MoveGroup {
    int kind, source, target;
    std::vector<Key> deadlines;
    auto operator<=>(const MoveGroup&) const = default;
};
struct RepairCandidate {
    CheapScore cheap;
    MoveGroup group;
    std::vector<int> detail;
    BundlePlan plan;
};
struct NeighborhoodResult {
    Count generated;
    std::vector<RepairCandidate> selected;
};

class RepairContext {
    NativeHandle scorer_{nullptr, candidate_scoring_destroy};
    NativeHandle order_{nullptr, deadline_order_destroy};
    NativeHandle propagation_{nullptr, route_propagation_destroy};
    NativeHandle strong_{nullptr, strong_deadline_destroy};
    // Destroy the neighborhood before the scorer and order it borrows.
    NativeHandle neighborhood_{nullptr, neighborhood_destroy};
    std::map<BundlePlan, CheapScore> cheap_cache_;
    std::map<BundlePlan, StrongScore> strong_cache_;
    std::map<BundleRoute, int> length_cache_;
public:
    const Bundles& bundles;
    bool use_seeds;
    std::vector<Key> keys;
    std::vector<std::vector<int>> members;
    std::map<Key, int> indices;
    std::vector<int> crop_order;
    BundlePlan initial;

    RepairContext(const Bundles& supplied, bool seeds)
        : bundles(supplied), use_seeds(seeds && !supplied.seed_supply.empty()) {
        const auto& routing = bundles.routing;
        const auto& tasks = routing.task_data.tasks;
        const auto& deadline = routing.task_data.fixed_deadline;
        const int n = tasks.size(), patterns = bundles.members.size();
        std::set<int> item_set;
        for (const auto& task : tasks) {
            if (task.input >= 0) item_set.insert(task.input);
            if (task.output >= 0) item_set.insert(task.output);
            if (seeds && task.crop >= 0 && bundles.seed_supply.contains(task.crop) && std::find(crop_order.begin(), crop_order.end(), task.crop) == crop_order.end())
                crop_order.push_back(task.crop);
        }
        std::vector<int> item_ids(item_set.begin(), item_set.end());
        auto item_index = [&](int item) { return item < 0 ? -1 : int(std::lower_bound(item_ids.begin(), item_ids.end(), item) - item_ids.begin()); };
        for (const auto& [key, ids] : bundles.members) {
            indices[key] = keys.size();
            keys.push_back(key);
            members.push_back(ids);
            std::sort(members.back().begin(), members.back().end());
        }
        for (const auto& route : bundles.routes) {
            BundleRoute ids;
            for (Key key : route) ids.push_back(indices.at(key));
            if (!ids.empty()) initial.push_back(ids);
        }
        std::map<Key, int> groups;
        // The Python deadline dictionary visits greedy delivery groups in
        // hour/item order, then tasks in each selected output list.
        for (int hour = 0; hour < hours; ++hour) for (int item = 0; item < items; ++item)
            if (auto found = routing.task_data.deliveries.find({item, hour}); found != routing.task_data.deliveries.end())
                for (int id : found->second) {
                    const Key key{tasks[id].output, hour};
                    if (!groups.contains(key)) groups[key] = groups.size();
                }
        std::vector<int> columns(12 * n), arrival(n), worker_data, ready;
        std::vector<Count> quantities, free, supply;
        for (const auto& worker : bundles.workers) {
            worker_data.insert(worker_data.end(), {worker.start[0], worker.start[1], worker.release});
        }
        for (int item : item_ids) {
            ready.push_back(routing.input_ready.contains(item) ? routing.input_ready.at(item) : 0);
            free.push_back(std::find(routing.input_items.begin(), routing.input_items.end(), item) == routing.input_items.end() ? 0 : routing.free_inputs[item]);
        }
        for (int crop : crop_order) {
            const auto& curve = bundles.seed_supply.at(crop);
            supply.insert(supply.end(), curve.begin(), curve.end());
        }
        for (const auto& task : tasks) {
            const int id = task.id;
            arrival[id] = 100;
            for (const auto& worker : bundles.workers) arrival[id] = std::min(arrival[id], worker.release + distance(worker.start, task.point));
            const auto crop = std::find(crop_order.begin(), crop_order.end(), task.crop);
            const auto due = deadline.find(id);
            const std::array<int, 12> values{
                task.point[0], task.point[1], task.predecessor, bundles.early[id], bundles.late[id], arrival[id],
                item_index(task.input), item_index(task.output), crop == crop_order.end() ? -1 : int(crop - crop_order.begin()),
                due == deadline.end() ? 0 : due->second, due == deadline.end() ? -1 : groups.at({task.output, due->second}), 1 + tail(task.point)};
            for (int row = 0; row < 12; ++row) columns[row * n + id] = values[row];
            quantities.push_back(task.quantity);
        }
        std::vector<int> flat, boundaries{0};
        for (const auto& ids : members) { flat.insert(flat.end(), ids.begin(), ids.end()); boundaries.push_back(flat.size()); }
        require(int(flat.size()) == n, "repair bundles must cover every task");
        std::vector<int> pattern_data = boundaries;
        for (const auto& ids : members) pattern_data.push_back(ids.front());
        for (const auto& ids : members) pattern_data.push_back(tail(tasks[ids.front()].point));
        const std::array<int, 6> sizes{n, patterns, int(item_ids.size()), int(crop_order.size()), int(bundles.capacities.size()), int(groups.size())};
        scorer_.reset(candidate_scoring_create(sizes.data(), buffer(columns), buffer(pattern_data), buffer(flat), buffer(bundles.capacities), buffer(quantities), buffer(free), buffer(supply)));
        require(bool(scorer_), "invalid native repair scoring context");
        std::vector<int> x, y, pred;
        for (const auto& task : tasks) { x.push_back(task.point[0]); y.push_back(task.point[1]); pred.push_back(task.predecessor); }
        propagation_.reset(route_propagation_create(n, buffer(x), buffer(y), buffer(pred), buffer(bundles.early), buffer(bundles.late), buffer(arrival)));
        require(bool(propagation_), "invalid native repair propagation context");
        std::vector<int> strong_columns(9 * n);
        const std::array<int, 9> mapping{0, 1, 2, 3, 4, 9, 6, 7, 5};
        for (int row = 0; row < 9; ++row) for (int id = 0; id < n; ++id)
            strong_columns[row * n + id] = row == 5 && !deadline.contains(id) ? -1 : columns[mapping[row] * n + id];
        const std::array<int, 3> strong_sizes{n, int(bundles.workers.size()), int(item_ids.size())};
        strong_.reset(strong_deadline_create(strong_sizes.data(), buffer(strong_columns), buffer(quantities), buffer(worker_data), buffer(ready)));
        require(bool(strong_), "invalid native repair deadline context");
        x.clear(); y.clear();
        std::vector<int> limits, edges, identity, offsets{0}, deadline_pairs;
        for (int index = 0; index < patterns; ++index) {
            const Key key = keys[index];
            const Point point = tasks[members[index].front()].point;
            x.push_back(point[0]); y.push_back(point[1]); identity.push_back(index);
            int limit = hours + 1;
            for (const auto& [other, ids] : bundles.members) if (other.first == key.first && other.second >= key.second)
                for (int task : ids) if (deadline.contains(task)) limit = std::min(limit, deadline.at(task));
            limits.push_back(limit);
            std::set<Key> pairs;
            for (int task : members[index]) if (deadline.contains(task)) pairs.insert({tasks[task].output, deadline.at(task)});
            for (const auto& [item, hour] : pairs) deadline_pairs.insert(deadline_pairs.end(), {item, hour});
            offsets.push_back(deadline_pairs.size() / 2);
        }
        for (Key a : keys) for (Key b : keys)
            edges.push_back((a.first == b.first && a.second < b.second) || (bundles.predecessors.contains(b) && bundles.predecessors.at(b).contains(a)));
        const Point start = *std::min_element(shed.begin(), shed.end());
        order_.reset(deadline_order_create(patterns, buffer(x), buffer(y), buffer(limits), buffer(edges), start[0], start[1]));
        require(bool(order_), "invalid native repair order context");
        neighborhood_.reset(neighborhood_create(scorer_.get(), order_.get(), patterns, buffer(identity), buffer(offsets), buffer(deadline_pairs)));
        require(bool(neighborhood_), "invalid native repair neighborhood context");
    }

    std::vector<int> pack(const BundlePlan& plan) const {
        std::vector<int> packed;
        for (const auto& route : plan) { packed.insert(packed.end(), route.begin(), route.end()); packed.push_back(-1); }
        return packed;
    }
    std::pair<std::vector<int>, std::vector<int>> task_plan(const BundlePlan& plan) const {
        std::vector<int> flat, offsets{0};
        for (const auto& route : plan) {
            for (int bundle : route) { const auto& ids = members.at(bundle); flat.insert(flat.end(), ids.begin(), ids.end()); }
            offsets.push_back(flat.size());
        }
        return {flat, offsets};
    }
    CheapScore cheap(const BundlePlan& plan) {
        if (auto found = cheap_cache_.find(plan); found != cheap_cache_.end()) return found->second;
        const auto packed = pack(plan);
        CheapScore result;
        require(!candidate_scoring_run(scorer_.get(), buffer(packed), packed.size(), result.data()), "invalid repair scoring partition");
        cheap_cache_[plan] = result;
        return result;
    }
    Propagation propagate(const BundlePlan& plan) {
        const auto [flat, offsets] = task_plan(plan);
        Propagation result{false, 0, std::vector<int>(bundles.early.size())};
        int dummy = 0;
        const int status = route_propagation_run(propagation_.get(), buffer(flat), flat.size(), buffer(offsets), plan.size(), nullptr,
                                                  result.lower.empty() ? &dummy : result.lower.data(), &result.lateness);
        require(status >= 0, "invalid repair propagation partition");
        result.cyclic = status != 0;
        if (result.cyclic) result.lower.clear();
        return result;
    }
    StrongScore strong(const BundlePlan& plan) {
        if (auto found = strong_cache_.find(plan); found != strong_cache_.end()) return found->second;
        const auto [flat, offsets] = task_plan(plan);
        std::vector<int> output(plan.size() + 2);
        require(!strong_deadline_run(strong_.get(), buffer(flat), flat.size(), buffer(offsets), plan.size(), output.data()), "invalid repair deadline partition");
        require(output[1] >= 0 && output[1] <= int(plan.size()), "invalid impossible route count");
        StrongScore result{output[0], output[1], {output.begin() + 2, output.begin() + 2 + output[1]}};
        strong_cache_[plan] = result;
        return result;
    }
    std::vector<int> matching(const BundlePlan& plan, const Propagation& bounds) {
        std::vector<int> result(plan.size(), -1);
        if (bounds.cyclic) return result;
        const auto [flat, offsets] = task_plan(plan);
        int dummy = -1;
        require(strong_deadline_matching(strong_.get(), buffer(flat), flat.size(), buffer(offsets), plan.size(), buffer(bounds.lower),
                                        result.empty() ? &dummy : result.data()) >= 0, "invalid repair worker matching");
        return result;
    }
    int length(const BundleRoute& route) {
        if (auto found = length_cache_.find(route); found != length_cache_.end()) return found->second;
        const int value = candidate_route_length(scorer_.get(), buffer(route), route.size());
        require(value >= 0, "invalid repair route length");
        length_cache_[route] = value;
        return value;
    }
    BundleRoute ordered(const BundleRoute& route) {
        BundleRoute result(route.size());
        int dummy = 0;
        require(!deadline_order_run(order_.get(), buffer(route), route.size(), result.empty() ? &dummy : result.data()), "cyclic repair route ordering");
        return result;
    }
    NeighborhoodResult neighbors(const BundlePlan& plan, const std::vector<int>& sources, const std::set<BundlePlan>& visited, int shortlist, double seconds = -1) {
        const auto packed = pack(plan);
        std::vector<int> previous;
        for (const auto& value : visited) { auto ids = pack(value); previous.insert(previous.end(), ids.begin(), ids.end()); previous.push_back(-2); }
        const int status = neighborhood_run(neighborhood_.get(), buffer(packed), packed.size(), buffer(sources), sources.size(), buffer(previous), previous.size(), shortlist, seconds, use_seeds);
        require(!status, "repair neighborhood failed: " + std::string(neighborhood_error(neighborhood_.get())));
        int count = 0, position = 0;
        const Count* values = neighborhood_result(neighborhood_.get(), &count);
        auto take = [&]() { require(position < count, "truncated native neighborhood output"); return values[position++]; };
        auto integer = [&]() { const Count value = take(); require(value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max(), "oversized native index"); return int(value); };
        NeighborhoodResult result{take(), {}};
        const int selected = integer();
        for (int i = 0; i < selected; ++i) {
            RepairCandidate candidate;
            for (auto& value : candidate.cheap) value = take();
            candidate.group.kind = integer(); candidate.group.source = integer(); candidate.group.target = integer();
            const int pairs = integer();
            for (int pair = 0; pair < pairs; ++pair) { const int item = integer(), hour = integer(); candidate.group.deadlines.emplace_back(item, hour); }
            const int detail = integer();
            for (int j = 0; j < detail; ++j) candidate.detail.push_back(integer());
            const int size = integer();
            BundleRoute route;
            for (int j = 0; j < size; ++j) {
                const int id = integer();
                if (id == -1) { candidate.plan.push_back(route); route.clear(); }
                else { require(id >= 0 && id < int(keys.size()), "invalid neighbor bundle index"); route.push_back(id); }
            }
            require(route.empty(), "unterminated neighbor route");
            result.selected.push_back(std::move(candidate));
        }
        require(position == count, "extra neighborhood output");
        return result;
    }
};
}  // namespace day_constructor
