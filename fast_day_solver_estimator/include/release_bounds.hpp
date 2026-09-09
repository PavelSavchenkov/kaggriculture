#pragma once
#include "bounds.hpp"

namespace labor {
struct ReleaseBound {
    int workers = 1;
    int seed_missing = 0;
    int land_missing = 0;
    int seed_late_actions = 0;
    int land_late_actions = 0;
    int tight_release = 0;
    int tight_radius = 0;
    float pressure = 0;
    friend bool operator==(const ReleaseBound&, const ReleaseBound&) = default;
};

// Global seeds and land allow prepositioning. Unlike carried inputs, neither
// requires a pickup after release. Each cut is an independent necessary bound.
inline ReleaseBound release_bound(const day_solver::DayProblem& p, const HireMenu& menu, int hours = 24) {
    if (hours != 23 && hours != 24) throw std::runtime_error("unsupported release horizon");
    std::array<std::array<int, kag::N_CROPS>, 11> plants{};
    std::array<std::array<int, 11>, 25> land_work{};
    std::array<std::array<int64_t, kag::N_CROPS>, 25> purchases{};
    auto seeds = p.start.seeds;
    std::array<int, 4> land_release; land_release.fill(hours + 1);
    for (const auto& e : p.market_plan) {
        if (e.market_op != kag::M_BUY_SEED && e.market_op != kag::M_BUY_LAND) continue;
        if (e.hour < 0 || e.hour >= hours) throw std::runtime_error("purchase outside active horizon");
        if (e.market_op == kag::M_BUY_SEED) purchases[e.hour + 1][e.item] += e.quantity;
        else land_release[e.item] = std::min(land_release[e.item], e.hour + 1);
    }
    ReleaseBound result;
    for (const auto& work : p.tile_work) {
        const auto& tile = p.start.managed_tiles[work.tile];
        const int radius = shed_distance(tile.y * 10 + tile.x);
        for (const auto& action : work.actions) if (action.op == kag::OP_PLANT)
            for (int r = 0; r <= radius; ++r) ++plants[r][action.arg];
        if (tile.state.kind != day_solver::ManagedTileKind::LOCKED) continue;
        const int release = land_release[kag::quadrant_of(tile.x, tile.y, kag::BOARD)];
        if (release >= hours) result.land_missing += work.actions.size();
        for (int t = 1; t <= std::min(release, hours); ++t)
            for (int r = 0; r <= radius; ++r) land_work[t][r] += work.actions.size();
    }
    auto cut = [&](int required, int release, int radius) {
        if (!required) return;
        const int farmer_room = std::max(0, hours - std::max(release, radius));
        result.pressure = std::max(result.pressure, float(required) / std::max(1, farmer_room));
        int workers = 1, capacity = farmer_room;
        while (capacity < required && workers < 40) {
            capacity += std::max(0, hours - std::max(release, menu.hours[workers - 1] + 1 + radius));
            ++workers;
        }
        if (capacity < required) workers = 41;
        if (workers > result.workers) {
            result.workers = workers; result.tight_release = release; result.tight_radius = radius;
        }
    };
    for (int t = 1; t <= hours; ++t) {
        const bool seed_release = std::any_of(purchases[t].begin(), purchases[t].end(), [](int64_t n) { return n > 0; });
        for (int r = 0; r <= 10; ++r) {
            if (seed_release) {
                int required = 0;
                for (int crop = 0; crop < kag::N_CROPS; ++crop)
                    required += std::max<int64_t>(0, plants[r][crop] - seeds[crop]);
                result.seed_late_actions = std::max(result.seed_late_actions, required);
                if (t == hours) result.seed_missing = std::max(result.seed_missing, required);
                cut(required, t, r);
            }
            result.land_late_actions = std::max(result.land_late_actions, land_work[t][r]);
            cut(land_work[t][r], t, r);
        }
        for (int crop = 0; crop < kag::N_CROPS; ++crop) seeds[crop] += purchases[t][crop];
    }
    int missing = 0;
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        missing += std::max<int64_t>(0, plants[0][crop] - seeds[crop]);
    result.seed_missing = std::max(result.seed_missing, missing);
    if (result.seed_missing || result.land_missing) result.workers = 41;
    return result;
}
}
