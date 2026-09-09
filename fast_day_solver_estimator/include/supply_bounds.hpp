#pragma once
#include "bounds.hpp"

namespace labor {
struct SupplyBound {
    int workers = 1;
    int missing = 0;
    int late_actions = 0;
    int tight_release = 0;
    int tight_radius = 0;
    float pressure = 0;
    friend bool operator==(const SupplyBound&, const SupplyBound&) = default;
};

// Necessary conditions only. All potential field production is free at dawn,
// withdrawals are ignored, and workers may start at any shed access square.
// A purchase at h is first available for pickup at h+1. Each worker consuming
// any such input at distance >=r needs a pickup and >=r travel actions first.
inline SupplyBound supply_bound(const day_solver::DayProblem& p, const HireMenu& menu, int hours = 24) {
    if (hours != 23 && hours != 24) throw std::runtime_error("unsupported supply horizon");
    std::array<std::array<int, kag::N_ITEMS>, 11> demand{};
    std::array<int64_t, kag::N_ITEMS> available{};
    std::array<std::array<int64_t, kag::N_ITEMS>, 25> purchases{};
    for (int item = 0; item < kag::N_ITEMS; ++item) available[item] = p.start.shed[item];
    for (const auto& work : p.tile_work) {
        const auto& tile = p.start.managed_tiles[work.tile];
        const int radius = shed_distance(tile.y * 10 + tile.x);
        for (const auto& a : work.actions) {
            const int item = a.op == kag::OP_FEED ? kag::WHEAT : a.op == kag::OP_FERTILIZE ? kag::FERTILIZER : a.op == kag::OP_PLACE ? a.arg : -1;
            if (item >= 0) for (int r = 0; r <= radius; ++r) ++demand[r][item];
            if (a.output_item >= 0) available[a.output_item] += a.output_quantity;
        }
    }
    for (const auto& e : p.market_plan)
        if (e.market_op == kag::M_BUY_PRODUCT || e.market_op == kag::M_BUY_ANIMAL) {
            if (e.hour < 0 || e.hour >= hours) throw std::runtime_error("purchase outside active horizon");
            purchases[e.hour + 1][e.item] += e.quantity;
        }
    SupplyBound result;
    for (int t = 1; t <= hours; ++t) {
        if (std::none_of(purchases[t].begin(), purchases[t].end(), [](int64_t n) { return n > 0; })) continue;
        for (int r = 0; r <= 10; ++r) {
            int required = 0;
            for (int item = 0; item < kag::N_ITEMS; ++item)
                required += std::max<int64_t>(0, demand[r][item] - available[item]);
            if (!required) continue;
            result.late_actions = std::max(result.late_actions, required);
            const int per_worker = std::max(0, hours - t - r - 1);
            if (!per_worker) result.missing = std::max(result.missing, required);
            result.pressure = std::max(result.pressure, float(required) / std::max(1, per_worker));
            int workers = 1, capacity = per_worker;
            while (capacity < required && workers < 40) {
                capacity += std::max(0, hours - std::max(t, menu.hours[workers - 1] + 1) - r - 1);
                ++workers;
            }
            if (capacity < required) workers = 41;
            if (workers > result.workers) {
                result.workers = workers; result.tight_release = t; result.tight_radius = r;
            }
        }
        for (int item = 0; item < kag::N_ITEMS; ++item) available[item] += purchases[t][item];
    }
    int shortage = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        shortage += std::max<int64_t>(0, demand[0][item] - available[item]);
    result.missing = std::max(result.missing, shortage);
    if (result.missing) result.workers = 41;
    return result;
}
}
