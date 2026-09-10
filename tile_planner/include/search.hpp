#pragma once
#include "course.hpp"
#include <chrono>
#include <random>
#include <set>

namespace placement {
struct LayoutHash {
    size_t operator()(const Layout& layout) const {
        uint64_t h = 1469598103934665603ULL;
        for (auto cell : layout) { h ^= cell; h *= 1099511628211ULL; }
        return h;
    }
};
struct Score {
    double cost = 0, workers = 0;
    int rejected = 0, weak = 0;
    std::array<double, 30> day_cost{}, day_workers{};
    std::array<int, 30> lower{};
};
class Evaluator {
    const Course& course;
    std::unordered_map<Layout, Score, LayoutHash> cache;
    struct DayScore { double cost, workers; int lower; bool rejected, weak; };
    std::array<std::unordered_map<Layout, DayScore, LayoutHash>, 30> day_cache;
    bool reuse_days;
public:
    int queries = 0, hits = 0, day_queries = 0, day_hits = 0, evictions = 0;
    explicit Evaluator(const Course& c, bool reuse = true) : course(c), reuse_days(reuse) { cache.reserve(4096); }
    const Score& operator()(const Layout& layout) {
        if (const auto it = cache.find(layout); it != cache.end()) { ++hits; return it->second; }
        if (!course.legal(layout)) throw std::runtime_error("illegal placement");
        std::array<Layout, 30> layouts; layouts.fill(layout);
        const auto score = series(layouts);
        if (cache.size() >= 20000) { cache.clear(); ++evictions; }
        return cache.emplace(layout, score).first->second;
    }
    // Caller validates dated continuity before scoring a series.
    Score series(const std::array<Layout, 30>& layouts) {
        Score score;
        for (int d = 0; d < 30; ++d) {
            const auto& layout = layouts[d];
            // Cache scope is one immutable course/day. The current estimator
            // reads coordinates only for tile_work; other managed tile states
            // contribute coordinate-independent counts. This key is ONLY for
            // estimates, never for schedule or complete-game certificates.
            Layout key; key.fill(255);
            for (const auto& work : course.days[d].problem.tile_work) key[work.tile] = layout[work.tile];
            DayScore value;
            const auto found = day_cache[d].find(key);
            if (reuse_days && found != day_cache[d].end()) { value = found->second; ++day_hits; }
            else {
                const auto estimate = fast_day_solver_estimator::estimate_day(course.problem(d, layout), course.days[d].menu);
                value = {estimate.analytically_rejected ? 1e12 : estimate.cost,
                         estimate.analytically_rejected ? 41 : estimate.workers,
                         estimate.lower, estimate.analytically_rejected, estimate.weak_probe || estimate.low_peak};
                ++day_queries;
                if (reuse_days) {
                    if (day_cache[d].size() >= 30000) { day_cache[d].clear(); ++evictions; }
                    day_cache[d].emplace(key, value);
                }
            }
            score.lower[d] = value.lower; score.rejected += value.rejected; score.weak += value.weak;
            score.day_cost[d] = value.cost; score.day_workers[d] = value.workers;
            score.cost += score.day_cost[d]; score.workers += score.day_workers[d];
        }
        ++queries;
        return score;
    }
};

// The rearrangement inequality solves this additive weighted-distance model
// exactly. It is a proposal, not an exact solution of shared worker routing.
inline Layout radial(const Course& course, const std::array<double, 100>& weight) {
    auto result = identity();
    for (int q = 0; q < 4; ++q) {
        std::vector<int> lives, cells;
        for (int i = 0; i < 100; ++i) if (quadrant(i) == q && !course.fixed[i]) { lives.push_back(i); cells.push_back(i); }
        std::stable_sort(lives.begin(), lives.end(), [&](int a, int b) { return weight[a] > weight[b]; });
        std::stable_sort(cells.begin(), cells.end(), [](int a, int b) {
            return std::pair{labor::shed_distance(a), a} < std::pair{labor::shed_distance(b), b};
        });
        for (size_t i = 0; i < lives.size(); ++i) result[lives[i]] = cells[i];
    }
    return result;
}

struct Candidate { std::string method; Layout layout; Score score; };
inline std::vector<Candidate> rules(const Course& course, Evaluator& evaluate) {
    std::vector<Candidate> result;
    auto add = [&](std::string name, Layout layout) {
        if (std::any_of(result.begin(), result.end(), [&](const auto& old) { return old.layout == layout; })) return;
        const auto score = evaluate(layout); result.push_back({std::move(name), layout, score});
    };
    add("source", identity());
    add("radial_frequency", radial(course, course.frequency));
    add("radial_actions", radial(course, course.actions));
    add("radial_io", radial(course, course.input_output));
    add("radial_shadow", radial(course, course.shadow));
    return result;
}

inline Candidate swap_search(const Course& course, Evaluator& evaluate, Candidate start,
                             double seconds, uint64_t seed, bool annealing, bool equal_radius = false) {
    const auto began = std::chrono::steady_clock::now();
    auto current = start, best = start;
    std::mt19937_64 rng(seed);
    std::vector<std::pair<int, int>> moves;
    for (int a = 0; a < 100; ++a) if (!course.fixed[a])
        for (int b = a + 1; b < 100; ++b) if (!course.fixed[b] && quadrant(a) == quadrant(b) &&
            (!equal_radius || labor::shed_distance(a) == labor::shed_distance(b)) &&
            (course.actions[a] || course.actions[b])) moves.emplace_back(a, b);
    if (moves.empty()) return start;
    int iteration = 0;
    for (;;) {
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        if (elapsed >= seconds) break;
        const auto [a, b] = moves[rng() % moves.size()];
        auto layout = current.layout; std::swap(layout[a], layout[b]);
        auto score = evaluate(layout);
        const double delta = score.cost - current.score.cost;
        const double temperature = std::max(.05, start.score.cost * .02 * std::pow(.01, elapsed / seconds));
        const bool accept = delta < -1e-9 || (std::abs(delta) < 1e-9 && score.workers < current.score.workers) ||
            (annealing && std::generate_canonical<double, 53>(rng) < std::exp(-std::max(0., delta) / temperature));
        if (accept) current = {"search", layout, score};
        if (std::pair{current.score.cost, current.score.workers} < std::pair{best.score.cost, best.score.workers}) best = current;
        ++iteration;
    }
    best.method = (annealing ? "anneal_" : "swap_") + start.method;
    return best;
}
}
