#pragma once
#include "fast_game_engine/sim.hpp"
#include <tuple>

namespace sales_planner {
inline auto physical_tile_fields(const kag::Tile& t) {
    return std::tie(t.kind, t.what, t.has_animal, t.watered_today, t.fed_today,
        t.cared_today, t.fertilizer_available, t.consecutive_dry, t.yield_units,
        t.pending_care_bonus, t.planted_day, t.max_lifespan_step, t.fertilized_until_day);
}

// Shed stock may differ during a delayed sale. Check terminal shed stock
// separately; cash, trade counters and market inventory are outside this test.
inline int physical_difference(const kag::Farm& a, const kag::Farm& b) {
    int mask = 0;
    if (a.n_units != b.n_units || a.n_quadrants != b.n_quadrants || a.hires_today != b.hires_today) mask |= 1;
    if (!std::equal(a.seeds, a.seeds + kag::N_CROPS, b.seeds)) mask |= 2;
    for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x)
        if (physical_tile_fields(a.tiles[y][x]) != physical_tile_fields(b.tiles[y][x])) mask |= 4;
    for (int u = 0; u < std::min(a.n_units, b.n_units); ++u) {
        if (a.pos_x[u] != b.pos_x[u] || a.pos_y[u] != b.pos_y[u]) mask |= 8;
        if (!std::equal(a.inv[u], a.inv[u] + kag::N_ITEMS, b.inv[u])) mask |= 16;
        if (a.inv_nkeys[u] != b.inv_nkeys[u] ||
            !std::equal(a.inv_keys[u], a.inv_keys[u] + a.inv_nkeys[u], b.inv_keys[u])) mask |= 32;
    }
    return mask;
}
}
