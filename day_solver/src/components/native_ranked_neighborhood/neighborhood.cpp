#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Reuse the existing kernels through their C APIs. Context ownership stays
// with the Python wrapper; no C++ object layout crosses a library boundary.
extern "C" {
int candidate_scoring_run(void*, const int*, int, std::int64_t*);
int candidate_route_length(void*, const int*, int);
int deadline_order_run(void*, const int*, int, int*);
}

namespace ranked_neighborhood {
using Route = std::vector<int>;
using Plan = std::vector<Route>;
using Deadline = std::pair<int, int>;
using Deadlines = std::set<Deadline>;
using Clock = std::chrono::steady_clock;

struct RouteHash {
    std::size_t operator()(const Route& route) const {
        std::size_t result = route.size();
        for (int pattern : route) result ^= std::size_t(pattern + 3) + 0x9e3779b9 + (result << 6) + (result >> 2);
        return result;
    }
};

struct Group {
    int kind, source, target;
    Deadlines deadlines;
    auto operator<=>(const Group&) const = default;
};

struct Candidate {
    Route signature;
    std::array<std::int64_t, 9> score;
    Group group;
    Route detail;
};

struct Neighborhood {
    void *scorer, *order;
    int patterns;
    bool identity_mapping = true;
    std::vector<int> scoring_index;
    std::vector<Deadlines> deadlines;
    std::unordered_map<Route, int, RouteHash> lengths;
    std::unordered_map<Route, Route, RouteHash> ordered;
    std::vector<std::int64_t> result;
    std::string error;

    Neighborhood(void* score, void* ordering, int count, const int* indices,
                 const int* offsets, const int* pairs)
        : scorer(score), order(ordering), patterns(count), scoring_index(indices, indices + count), deadlines(count) {
        if (!scorer || !order || count < 0 || offsets[0] != 0) throw std::runtime_error("invalid neighborhood context");
        auto permutation = scoring_index;
        std::sort(permutation.begin(), permutation.end());
        for (int p = 0; p < patterns; ++p) {
            identity_mapping &= scoring_index[p] == p;
            if (permutation[p] != p || offsets[p] > offsets[p + 1]) throw std::runtime_error("invalid pattern mapping");
            for (int i = offsets[p]; i < offsets[p + 1]; ++i) deadlines[p].insert({pairs[2 * i], pairs[2 * i + 1]});
        }
    }

    static Route pack(const Plan& plan) {
        Route packed;
        size_t size = plan.size();
        for (const auto& route : plan) size += route.size();
        packed.reserve(size);
        for (const auto& route : plan) {
            packed.insert(packed.end(), route.begin(), route.end());
            packed.push_back(-1);
        }
        return packed;
    }

    Plan unpack(const int* packed, int count) const {
        Plan plan;
        Route route;
        std::vector<bool> seen(patterns);
        int visited = 0;
        for (int i = 0; i < count; ++i) {
            const int p = packed[i];
            if (p == -1) {
                if (route.empty()) throw std::runtime_error("empty neighborhood route");
                plan.push_back(std::move(route));
                route.clear();
            } else {
                if (p < 0 || p >= patterns || seen[p]) throw std::runtime_error("invalid neighborhood partition");
                seen[p] = true;
                ++visited;
                route.push_back(p);
            }
        }
        if (!route.empty() || visited != patterns) throw std::runtime_error("incomplete neighborhood partition");
        return plan;
    }

    Route remap(Route route) const {
        for (int& p : route) if (p >= 0) p = scoring_index[p];
        return route;
    }

    int length(const Route& route) {
        if (const auto found = lengths.find(route); found != lengths.end()) return found->second;
        const auto mapped = identity_mapping ? Route{} : remap(route);
        const auto& input = identity_mapping ? route : mapped;
        const int value = candidate_route_length(scorer, input.data(), input.size());
        if (value < 0) throw std::runtime_error("native route length rejected candidate");
        if (lengths.size() >= 16384) lengths.clear();
        lengths.emplace(route, value);
        return value;
    }

    Route reorder(const Route& route) {
        if (const auto found = ordered.find(route); found != ordered.end()) return found->second;
        Route value(route.size());
        if (deadline_order_run(order, route.data(), route.size(), value.data()))
            throw std::runtime_error("duplicate patterns or cyclic route precedence");
        if (ordered.size() >= 16384) ordered.clear();
        ordered.emplace(route, value);
        return value;
    }

    static Route insert(Route route, int position, const Route& moved) {
        route.insert(route.begin() + position, moved.begin(), moved.end());
        return route;
    }

    void run(const int* packed, int count, const int* sources, int source_count,
             const int* previous, int previous_count, int shortlist, double seconds, bool seed_supply) {
        const auto started = Clock::now();
        auto expired = [&] { return seconds >= 0 && std::chrono::duration<double>(Clock::now() - started).count() >= seconds; };
        const auto current = unpack(packed, count);
        std::unordered_set<Route, RouteHash> used;
        int begin = 0;
        for (int i = 0; i < previous_count; ++i) if (previous[i] == -2) {
            unpack(previous + begin, i - begin);
            used.emplace(previous + begin, previous + i);
            begin = i + 1;
        }
        if (begin != previous_count) throw std::runtime_error("unterminated visited plan");
        std::vector<Candidate> pool;
        auto add = [&](const Plan& plan, Group group, Route detail) {
            if (expired()) return;
            auto signature = pack(plan);
            if (!used.insert(signature).second) return;
            const auto mapped = identity_mapping ? Route{} : remap(signature);
            const auto& input = identity_mapping ? signature : mapped;
            std::array<std::int64_t, 9> score;
            if (candidate_scoring_run(scorer, input.data(), input.size(), score.data()))
                throw std::runtime_error("native scoring rejected candidate");
            pool.push_back({std::move(signature), score, std::move(group), std::move(detail)});
        };
        auto candidate = current;
        for (int source_index = 0; source_index < source_count; ++source_index) {
            const int source = sources[source_index];
            if (source < 0 || source >= int(current.size())) throw std::runtime_error("invalid source route");
            const auto& route = current[source];
            for (int first = 0; first < int(route.size()); ++first)
                for (int width = 1; width <= std::min(3, int(route.size()) - first); ++width) {
                    if (width == int(route.size())) continue;
                    Route moved(route.begin() + first, route.begin() + first + width), kept = route;
                    kept.erase(kept.begin() + first, kept.begin() + first + width);
                    Deadlines moved_deadlines;
                    for (int pattern : moved) moved_deadlines.insert(deadlines[pattern].begin(), deadlines[pattern].end());
                    for (int target = 0; target < int(current.size()); ++target) {
                        if (target == source) continue;
                        const auto& other = current[target];
                        std::vector<std::pair<int, int>> positions_by_length;
                        for (int position = 0; position <= int(other.size()); ++position)
                            positions_by_length.emplace_back(length(insert(other, position, moved)), position);
                        std::partial_sort(positions_by_length.begin(), positions_by_length.begin() + std::min<size_t>(2, positions_by_length.size()), positions_by_length.end());
                        std::set<int> positions;
                        for (int i = 0; i < std::min(2, int(positions_by_length.size())); ++i) positions.insert(positions_by_length[i].second);
                        for (int position = 0; position < int(other.size()); ++position)
                            for (const auto& deadline : deadlines[other[position]])
                                if (moved_deadlines.contains(deadline)) { positions.insert(position); break; }
                        for (int position : positions) {
                            Deadlines shared;
                            if (position < int(other.size()))
                                std::set_intersection(moved_deadlines.begin(), moved_deadlines.end(),
                                    deadlines[other[position]].begin(), deadlines[other[position]].end(), std::inserter(shared, shared.end()));
                            candidate[source] = kept;
                            candidate[target] = insert(other, position, moved);
                            const Route detail{source, first, width, target, position};
                            add(candidate, {shared.empty() ? 2 : 0, source, target, shared}, detail);
                            candidate[source] = reorder(candidate[source]);
                            candidate[target] = reorder(candidate[target]);
                            add(candidate, {1, source, target, {}}, detail);
                            candidate[source] = route;
                            candidate[target] = other;
                        }
                    }
                }
            for (int first = 0; first < int(route.size()); ++first)
                for (int position = 0; position < int(route.size()) + int(seed_supply); ++position) {
                    if (position == first || position == first + 1) continue;
                    candidate[source].erase(candidate[source].begin() + first);
                    candidate[source].insert(candidate[source].begin() + position - int(position > first), route[first]);
                    add(candidate, {3, source, source, {}}, {source, first, position});
                    candidate[source] = route;
                }
            for (int first = 0; first < int(route.size()); ++first)
                for (int target = 0; target < int(current.size()); ++target) {
                    if (target == source) continue;
                    const auto& other = current[target];
                    std::vector<std::pair<int, int>> partners;
                    for (int index = 0; index < int(other.size()); ++index) {
                        Route a = route, b = other;
                        std::swap(a[first], b[index]);
                        partners.emplace_back(length(a) + length(b), index);
                    }
                    std::partial_sort(partners.begin(), partners.begin() + std::min<size_t>(2, partners.size()), partners.end());
                    for (int i = 0; i < std::min(2, int(partners.size())); ++i) {
                        const int second = partners[i].second;
                        std::swap(candidate[source][first], candidate[target][second]);
                        add(candidate, {4, source, target, {}}, {source, first, target, second});
                        std::swap(candidate[source][first], candidate[target][second]);
                    }
                }
            if (expired()) break;
        }
        auto less = [&](int first, int second) {
            const auto& a = pool[first];
            const auto& b = pool[second];
            return std::tie(a.score, a.detail, a.signature) < std::tie(b.score, b.detail, b.signature);
        };
        auto keep_best = [&](std::vector<int>& indices, int count) {
            if (int(indices.size()) > count) {
                std::nth_element(indices.begin(), indices.begin() + count, indices.end(), less);
                indices.resize(count);
            }
            std::sort(indices.begin(), indices.end(), less);
        };
        std::vector<int> selected(pool.size());
        std::iota(selected.begin(), selected.end(), 0);
        keep_best(selected, std::max(1, shortlist / 2));
        std::vector<bool> taken(pool.size());
        for (int index : selected) taken[index] = true;
        std::map<Group, std::vector<int>> by_group;
        for (int i = 0; i < int(pool.size()); ++i) by_group[pool[i].group].push_back(i);
        // An item at group rank >= shortlist cannot be reached before the
        // selection fills: earlier group items are either selected here or
        // already occupy a global-best slot. Preserve the original tie order.
        for (auto& [group, indices] : by_group) keep_best(indices, std::max(1, shortlist));
        int rank = 0;
        while (int(selected.size()) < shortlist && !by_group.empty()) {
            for (auto entry = by_group.begin(); entry != by_group.end();) {
                if (rank >= int(entry->second.size())) { entry = by_group.erase(entry); continue; }
                const int index = entry->second[rank];
                if (!taken[index]) {
                    selected.push_back(index);
                    taken[index] = true;
                    if (int(selected.size()) >= shortlist) break;
                }
                ++entry;
            }
            ++rank;
        }
        result = {std::int64_t(pool.size()), std::int64_t(selected.size())};
        for (int index : selected) {
            const auto& candidate = pool[index];
            result.insert(result.end(), candidate.score.begin(), candidate.score.end());
            const auto& group = candidate.group;
            result.insert(result.end(), {group.kind, group.source, group.target, std::int64_t(group.deadlines.size())});
            for (const auto& [item, hour] : group.deadlines) result.insert(result.end(), {item, hour});
            result.push_back(candidate.detail.size());
            result.insert(result.end(), candidate.detail.begin(), candidate.detail.end());
            result.push_back(candidate.signature.size());
            result.insert(result.end(), candidate.signature.begin(), candidate.signature.end());
        }
    }
};

extern "C" {
void* ranked_neighborhood_create(void* scorer, void* order, int patterns, const int* indices, const int* offsets, const int* pairs) {
    if (patterns < 0) return nullptr;
    try { return new Neighborhood(scorer, order, patterns, indices, offsets, pairs); }
    catch (const std::exception&) { return nullptr; }
}

int ranked_neighborhood_run(void* context, const int* current, int count, const int* sources, int source_count,
                     const int* visited, int visited_count, int shortlist, double seconds, int seed_supply) {
    auto& neighborhood = *static_cast<Neighborhood*>(context);
    neighborhood.result.clear();
    neighborhood.error.clear();
    try { neighborhood.run(current, count, sources, source_count, visited, visited_count, shortlist, seconds, seed_supply); }
    catch (const std::exception& error) { neighborhood.error = error.what(); return -1; }
    return 0;
}

const std::int64_t* ranked_neighborhood_result(void* context, int* count) {
    const auto& result = static_cast<Neighborhood*>(context)->result;
    *count = result.size();
    return result.data();
}

const char* ranked_neighborhood_error(void* context) { return static_cast<Neighborhood*>(context)->error.c_str(); }
void ranked_neighborhood_destroy(void* context) { delete static_cast<Neighborhood*>(context); }
}

} // namespace ranked_neighborhood
