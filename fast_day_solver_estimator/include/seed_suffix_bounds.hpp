#pragma once
#include "bounds.hpp"

namespace labor {
struct SeedSuffixBound {
    int workers = 1;
    int late_actions = 0;
    int tight_release = 0;
    int tight_radius = 0;
    float pressure = 0;
    friend bool operator==(const SeedSuffixBound&, const SeedSuffixBound&) = default;
};

// For each tile, every action from its first planting onward must follow that
// planting's seed release. Give earlier seeds to the largest such suffixes:
// this deliberately minimizes the work left after a release. Tiles contribute
// once, so different crops cannot double-count their following actions.
inline SeedSuffixBound seed_suffix_bound(const day_solver::DayProblem& p, const HireMenu& menu, int hours = 24) {
    if (hours != 23 && hours != 24) throw std::runtime_error("unsupported seed suffix horizon");
    struct Suffix { int crop, radius, actions; };
    std::array<Suffix, 100> suffixes{};
    std::array<bool, 100> seen{};
    int count = 0;
    std::array<int, kag::N_CROPS> plant_tiles{};
    for (const auto& work : p.tile_work) {
        if (work.tile < 0 || work.tile >= int(p.start.managed_tiles.size()) || work.tile >= 100 || seen[work.tile])
            throw std::runtime_error("seed suffix requires unique valid tile work");
        seen[work.tile] = true;
        for (int i = 0; i < int(work.actions.size()); ++i) {
            const auto& action = work.actions[i];
            if (action.op != kag::OP_PLANT) continue;
            if (action.arg < 0 || action.arg >= kag::N_CROPS) throw std::runtime_error("invalid planting crop");
            const auto& tile = p.start.managed_tiles[work.tile];
            suffixes[count++] = {action.arg, shed_distance(tile.y * 10 + tile.x), int(work.actions.size()) - i};
            ++plant_tiles[action.arg];
            break;
        }
    }
    SeedSuffixBound result;
    if (!count) return result;
    std::sort(suffixes.begin(), suffixes.begin() + count, [](const auto& a, const auto& b) { return a.actions > b.actions; });
    std::array<std::array<int64_t, kag::N_CROPS>, 25> purchases{};
    auto available = p.start.seeds;
    for (const auto& e : p.market_plan) if (e.market_op == kag::M_BUY_SEED) {
        if (e.hour < 0 || e.hour >= hours || e.item < 0 || e.item >= kag::N_CROPS || e.quantity < 0)
            throw std::runtime_error("invalid seed purchase");
        purchases[e.hour + 1][e.item] += e.quantity;
    }
    for (int t = 1; t <= hours; ++t) {
        if (t != hours && std::none_of(purchases[t].begin(), purchases[t].end(), [](int64_t n) { return n > 0; })) continue;
        bool shortage = false;
        for (int crop = 0; crop < kag::N_CROPS; ++crop) shortage |= plant_tiles[crop] > available[crop];
        if (shortage) for (int r = 0; r <= 10; ++r) {
            auto early = available;
            int required = 0;
            for (int i = 0; i < count; ++i) if (suffixes[i].radius >= r) {
                const auto& suffix = suffixes[i];
                if (early[suffix.crop] > 0) --early[suffix.crop];
                else required += suffix.actions;
            }
            if (!required) continue;
            result.late_actions = std::max(result.late_actions, required);
            int workers = 1, capacity = std::max(0, hours - std::max(t, r));
            result.pressure = std::max(result.pressure, float(required) / std::max(1, capacity));
            while (capacity < required && workers < 40) {
                capacity += std::max(0, hours - std::max(t, menu.hours[workers - 1] + 1 + r));
                ++workers;
            }
            if (capacity < required) workers = 41;
            if (workers > result.workers) {
                result.workers = workers; result.tight_release = t; result.tight_radius = r;
            }
        }
        for (int crop = 0; crop < kag::N_CROPS; ++crop) available[crop] += purchases[t][crop];
    }
    return result;
}
}
