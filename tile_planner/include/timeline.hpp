#pragma once
#include "course.hpp"

namespace placement {
using Timeline = std::array<Layout, 30>;
struct SuffixSwap { int day, first, second; };

inline bool reassignable(const day_solver::ManagedTileState& state) {
    using K = day_solver::ManagedTileKind;
    return state.kind == K::EMPTY || state.kind == K::WEED || state.kind == K::LOCKED ||
           (day_solver::animal_structure(state.kind) && state.animal < 0);
}

inline bool legal_timeline(const Course& course, const Timeline& timeline) {
    if (!course.legal(timeline[0])) return false;
    for (int d = 1; d < 30; ++d) {
        std::array<int, 100> inverse{}; std::array<bool, 100> seen{};
        for (int i = 0; i < 100; ++i) inverse[timeline[d - 1][i]] = i;
        const auto& tiles = course.days[d].problem.start.managed_tiles;
        for (int i = 0; i < 100; ++i) {
            const int target = timeline[d][i];
            if (target >= 100 || seen[target]) return false;
            seen[target] = true;
            if (target == timeline[d - 1][i]) continue;
            const auto& state = tiles[i].state;
            if (!reassignable(state) || state != tiles[inverse[target]].state) return false;
            if (state.kind == day_solver::ManagedTileKind::LOCKED && quadrant(target) != quadrant(timeline[d - 1][i])) return false;
        }
    }
    return true;
}

inline Timeline make_timeline(const Course& course, std::vector<SuffixSwap> moves, Layout base = identity()) {
    std::stable_sort(moves.begin(), moves.end(), [](const auto& a, const auto& b) { return a.day < b.day; });
    for (const auto& move : moves) {
        if (move.day < 0 || move.day >= 30 || move.first < 0 || move.first >= 100 || move.second < 0 || move.second >= 100)
            throw std::runtime_error("invalid suffix swap");
        const auto& tiles = course.days[move.day].problem.start.managed_tiles;
        if (!reassignable(tiles[move.first].state) || tiles[move.first].state != tiles[move.second].state)
            throw std::runtime_error("suffix swap changes the existing farm state");
        if (tiles[move.first].state.kind == day_solver::ManagedTileKind::LOCKED && quadrant(move.first) != quadrant(move.second))
            throw std::runtime_error("suffix swap crosses locked quadrants");
    }
    Timeline result; size_t next = 0;
    for (int d = 0; d < 30; ++d) {
        while (next < moves.size() && moves[next].day == d) {
            std::swap(base[moves[next].first], base[moves[next].second]); ++next;
        }
        result[d] = base;
    }
    if (!legal_timeline(course, result)) throw std::runtime_error("illegal placement timeline");
    return result;
}

inline Timeline read_timeline(const fs::path& path) {
    std::ifstream input(path); std::vector<int> values; int value;
    while (input >> value) {
        if (value < 0 || value >= 100 || values.size() >= 3000) throw std::runtime_error("invalid timeline coordinate");
        values.push_back(value);
    }
    if (!input.eof() || (values.size() != 100 && values.size() != 3000)) throw std::runtime_error("timeline must contain 100 or 3000 cells");
    Timeline result;
    for (int d = 0; d < 30; ++d) for (int i = 0; i < 100; ++i) result[d][i] = values[values.size() == 100 ? i : d * 100 + i];
    return result;
}

inline void save_timeline(const Timeline& timeline, const fs::path& path) {
    std::ofstream output(path);
    for (const auto& layout : timeline) { for (auto cell : layout) output << int(cell) << ' '; output << '\n'; }
}
}
