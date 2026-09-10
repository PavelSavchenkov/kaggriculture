#pragma once
#include "timeline.hpp"
#include "search.hpp"

namespace placement {
struct DatedCandidate { std::string method; Timeline timeline; std::vector<SuffixSwap> moves; Score score; };

inline std::vector<SuffixSwap> dated_moves(const Course& course, bool equal_radius, bool cross_quadrants) {
    std::vector<SuffixSwap> moves;
    for (int d = 0; d < 30; ++d) {
        const auto& tiles = course.days[d].problem.start.managed_tiles;
        for (int a = 0; a < 100; ++a) if (reassignable(tiles[a].state))
            for (int b = a + 1; b < 100; ++b) {
                if (tiles[a].state != tiles[b].state || !(course.tasks[d][a] || course.tasks[d][b])) continue;
                if (equal_radius && labor::shed_distance(a) != labor::shed_distance(b)) continue;
                if ((!cross_quadrants || d == 0 || tiles[a].state.kind == day_solver::ManagedTileKind::LOCKED) && quadrant(a) != quadrant(b)) continue;
                moves.push_back({d, a, b});
            }
    }
    return moves;
}

inline DatedCandidate dated_search(const Course& course, Evaluator& evaluate, double seconds, uint64_t seed,
                                   bool equal_radius, bool cross_quadrants) {
    const auto moves = dated_moves(course, equal_radius, cross_quadrants);
    const auto original = make_timeline(course, {});
    DatedCandidate best{"dated", original, {}, evaluate.series(original)}, current = best;
    if (moves.empty()) return best;
    std::mt19937_64 rng(seed);
    const auto began = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count() < seconds) {
        auto genes = current.moves;
        const int operation = rng() % 8;
        if (operation == 0) genes.clear();
        if (!genes.empty() && operation <= 2) genes.erase(genes.begin() + rng() % genes.size());
        if (genes.size() < 4 && operation != 2) genes.push_back(moves[rng() % moves.size()]);
        else if (!genes.empty()) genes[rng() % genes.size()] = moves[rng() % moves.size()];
        std::stable_sort(genes.begin(), genes.end(), [](const auto& a, const auto& b) { return a.day < b.day; });
        Timeline timeline; auto layout = identity(); size_t next = 0;
        for (int d = 0; d < 30; ++d) {
            while (next < genes.size() && genes[next].day == d) {
                std::swap(layout[genes[next].first], layout[genes[next].second]); ++next;
            }
            timeline[d] = layout;
        }
        if (!legal_timeline(course, timeline)) continue;
        const auto score = evaluate.series(timeline);
        const auto rank = [](const Score& s) { return std::pair{s.cost, s.workers}; };
        if (rank(score) < rank(current.score) || (rank(score) == rank(current.score) && genes.size() < current.moves.size()))
            current = {"dated", timeline, genes, score};
        if (rank(current.score) < rank(best.score)) best = current;
        // Independent short starts keep the four-move search from requiring
        // an uphill step to discover a different combination.
        if (rng() % 300 == 0) current = {"dated", original, {}, evaluate.series(original)};
    }
    best.method = equal_radius ? "dated_equal_radius" : cross_quadrants ? "dated_owned_cross_quadrant" : "dated_same_quadrant";
    return best;
}
}
