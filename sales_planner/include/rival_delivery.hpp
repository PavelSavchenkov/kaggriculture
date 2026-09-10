#pragma once
#include "rival_stock.hpp"

namespace sales_planner {

// Earliest turn on which the rival could sell each nonbuyable output, using
// only public positions/ready yield and a conservative bound on held stock.
// The bound stops at night, when new yield and automatic deposits are possible.
inline std::array<int, kag::N_PRODUCTS> rival_sale_not_before(
        const kag::agent::AgentObservation& obs, const RivalStockHistory& history,
        const MarketRules& rules = {}) {
    if (rules.turns_per_day != 24 || kag::BOARD != 10) std::abort();
    const int night = (obs.step / 24 + 1) * 24;
    std::array<int, kag::N_PRODUCTS> result;
    result.fill(night);
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        if (!output_product(item) || history.upper[item]) result[item] = obs.step;
    const auto& rival = obs.opponent();
    for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
        const auto& tile = rival.tiles[y][x];
        if (tile.yield_units <= 0 || (tile.kind != kag::T_PLANT && !tile.has_animal)) continue;
        const int item = tile.has_animal ? kag::ANIMALS[tile.what - kag::GOOSE].product : tile.what;
        if (!output_product(item) || result[item] == obs.step) continue;
        if (!tile.has_animal && obs.day - tile.planted_day < kag::CROPS[item].first_yield_day) continue;
        const int shed = std::min(std::abs(x - 4), std::abs(x - 5)) +
                         std::min(std::abs(y - 4), std::abs(y - 5));
        // Hire now, act next turn; optimistic spawn at the closest shed tile.
        int distance = 2 * shed + 2;
        for (int u = 0; u < rival.n_units; ++u)
            distance = std::min(distance, std::abs(int(rival.pos_x[u]) - x) +
                std::abs(int(rival.pos_y[u]) - y) + shed + 1);
        result[item] = std::min(result[item], obs.step + distance);
    }
    return result;
}
}
