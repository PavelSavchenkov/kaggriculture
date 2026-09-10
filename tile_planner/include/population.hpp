#pragma once
#include "search.hpp"

namespace placement {
// Exact minimum-cost assignment for one <=25-cell quadrant. It solves only
// the donor-profile matching proposal, not the physical routing objective.
inline std::vector<int> assign(const std::vector<std::vector<double>>& costs) {
    const int n = costs.size();
    std::vector<double> u(n + 1), v(n + 1);
    std::vector<int> p(n + 1), way(n + 1);
    for (int i = 1; i <= n; ++i) {
        p[0] = i; int j0 = 0;
        std::vector<double> minimum(n + 1, 1e100); std::vector<bool> used(n + 1);
        do {
            used[j0] = true; const int i0 = p[j0]; double delta = 1e100; int j1 = 0;
            for (int j = 1; j <= n; ++j) if (!used[j]) {
                const double current = costs[i0 - 1][j - 1] - u[i0] - v[j];
                if (current < minimum[j]) { minimum[j] = current; way[j] = j0; }
                if (minimum[j] < delta) { delta = minimum[j]; j1 = j; }
            }
            for (int j = 0; j <= n; ++j) if (used[j]) { u[p[j]] += delta; v[j] -= delta; } else minimum[j] -= delta;
            j0 = j1;
        } while (p[j0]);
        do { const int j1 = way[j0]; p[j0] = p[j1]; j0 = j1; } while (j0);
    }
    std::vector<int> result(n);
    for (int j = 1; j <= n; ++j) result[p[j] - 1] = j - 1;
    return result;
}

inline Layout donor_layout(const Course& course, const Course& donor, double calendar_weight, bool equal_radius = false) {
    auto layout = identity();
    for (int q = 0; q < 4; ++q) {
        std::vector<int> cells;
        for (int i = 0; i < 100; ++i) if (quadrant(i) == q && !course.fixed[i]) cells.push_back(i);
        std::vector<std::vector<double>> costs(cells.size(), std::vector<double>(cells.size()));
        for (size_t i = 0; i < cells.size(); ++i) for (size_t j = 0; j < cells.size(); ++j) {
            double value = 0;
            for (int d = 0; d < 30; ++d) {
                const auto& a = course.days[d].problem.start.managed_tiles[cells[i]].state;
                const auto& b = donor.days[d].problem.start.managed_tiles[cells[j]].state;
                const int species_a = a.animal >= 0 ? a.animal : a.crop;
                const int species_b = b.animal >= 0 ? b.animal : b.crop;
                value += 3 * ((a.animal >= 0) != (b.animal >= 0)) + (species_a != species_b);
                value += calendar_weight * std::abs(course.tasks[d][cells[i]] - donor.tasks[d][cells[j]]);
            }
            costs[i][j] = equal_radius && labor::shed_distance(cells[i]) != labor::shed_distance(cells[j]) ? 1e12 : value + (cells[i] != cells[j]) * .001;
        }
        const auto matching = assign(costs);
        for (size_t i = 0; i < cells.size(); ++i) layout[cells[i]] = cells[matching[i]];
    }
    return layout;
}

inline Candidate population_search(const Course& course, Evaluator& evaluate, std::vector<Candidate> seeds,
                                   double seconds, uint64_t seed, bool blocks, bool equal_radius = false) {
    std::mt19937_64 rng(seed);
    std::vector<Candidate> population = std::move(seeds);
    auto rank = [](const auto& a, const auto& b) { return std::pair{a.score.cost, a.score.workers} < std::pair{b.score.cost, b.score.workers}; };
    std::sort(population.begin(), population.end(), rank);
    Candidate best = population.front();
    std::array<std::vector<std::pair<int, int>>, 4> moves;
    std::vector<int> active_quadrants;
    for (int a = 0; a < 100; ++a) if (!course.fixed[a])
        for (int b = a + 1; b < 100; ++b) if (!course.fixed[b] && quadrant(a) == quadrant(b) && (course.actions[a] || course.actions[b]) &&
            (!equal_radius || labor::shed_distance(a) == labor::shed_distance(b)))
            moves[quadrant(a)].emplace_back(a, b);
    for (int q = 0; q < 4; ++q) if (!moves[q].empty()) active_quadrants.push_back(q);
    if (active_quadrants.empty()) return best;
    const auto began = std::chrono::steady_clock::now();
    int iteration = 0;
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count() < seconds) {
        auto parent = [&] {
            int chosen = rng() % population.size();
            for (int i = 0; i < 2; ++i) { const int candidate = rng() % population.size(); if (rank(population[candidate], population[chosen])) chosen = candidate; }
            return chosen;
        };
        auto layout = population[parent()].layout;
        const int block = active_quadrants[blocks ? (iteration / 80) % active_quadrants.size() : rng() % active_quadrants.size()];
        if (rng() % 3 == 0) {
            const auto& donor = population[parent()].layout;
            for (int i = 0; i < 100; ++i) if (quadrant(i) == block) layout[i] = donor[i];
        }
        const auto [a, b] = moves[block][rng() % moves[block].size()];
        std::swap(layout[a], layout[b]);
        const auto score = evaluate(layout);
        Candidate candidate{blocks ? "quadrant_population" : "global_population", layout, score};
        if (rank(candidate, best)) best = candidate;
        if (std::none_of(population.begin(), population.end(), [&](const auto& old) { return old.layout == layout; })) {
            population.push_back(std::move(candidate)); std::sort(population.begin(), population.end(), rank);
            if (population.size() > 24) {
                // Retain four elites; stochastic replacement keeps alternative
                // donor/quadrant combinations available during short runs.
                population.erase(population.begin() + 4 + rng() % (population.size() - 4));
            }
        }
        ++iteration;
    }
    best.method = blocks ? "quadrant_population" : "global_population";
    return best;
}
}
