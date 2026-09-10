#pragma once
#include "calendar.hpp"
#include "agents/common/api/agent_api.hpp"

namespace sales_planner {
// Exact own inventory projection through worker actions. Build a private local
// simulator from this observation only, with an arbitrary seed and PASS rival.
// Suppress market orders and the day-end update. Town/decay after worker actions
// cannot change shed, seed or carried quantities, which are the only outputs.
// This is not a projected full game state and must not be used as one.
inline bool project_resources(const kag::agent::AgentObservation& obs, const kag::Action& action,
                              Account& account, Resources& resources, int capacity = 100,
                              int turns_per_day = 24) {
    if (turns_per_day <= 1) return false;
    kag::Config config;
    config.seed = 0; config.shed_capacity = capacity; config.turns_per_day = turns_per_day;
    kag::Sim projection(config);
    projection.st.step = obs.step; projection.st.day = obs.day;
    projection.st.hour = 0; // Suppress day-end deposits; retain the actual action day.
    auto& f = projection.st.farms[0];
    f = {};
    const auto& pub = obs.self();
    f.money = pub.money; f.n_units = pub.n_units;
    f.hires_today = pub.hires_today; f.n_quadrants = pub.n_quadrants;
    std::copy_n(pub.pos_x, kag::MAX_UNITS, f.pos_x);
    std::copy_n(pub.pos_y, kag::MAX_UNITS, f.pos_y);
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            const auto tile = f.tiles[y][x] = pub.tiles[y][x];
            const int index = y * kag::BOARD + x, word = index / 64;
            const uint64_t bit = uint64_t{1} << (index % 64);
            if (tile.kind == kag::T_EMPTY) f.empty_mask[word] |= bit;
            if (tile.kind == kag::T_PLANT) {
                f.plant_mask[word] |= bit;
                if (tile.max_lifespan_step >= 0) {
                    f.decay_mask[word] |= bit;
                    f.next_decay_step = std::min(f.next_decay_step, int(tile.max_lifespan_step));
                }
            }
            if (tile.has_animal) f.animal_mask[word] |= bit;
        }
    std::copy_n(obs.own.shed, kag::N_ITEMS, f.shed);
    std::copy_n(obs.own.seeds, kag::N_CROPS, f.seeds);
    f.shed_total = obs.own.shed_total;
    for (int u = 0; u < f.n_units; ++u)
        for (int k = 0; k < obs.own.inv_nkeys[u]; ++k) {
            const int item = obs.own.inv_keys[u][k];
            f.inv_add(u, item, obs.own.inv[u][item]);
        }
    auto work = action; work.n_orders = 0; work.finalize();
    kag::Action pass; pass.clear(); pass.n_units = 1; pass.finalize();
    projection.step(work, pass);
    account = {}; resources = {};
    account.cash = f.money; account.units = f.n_units; account.total = f.shed_total;
    account.hires = f.hires_today; account.quadrants = f.n_quadrants;
    std::copy_n(f.shed, kag::N_ITEMS, account.stock.begin());
    std::copy_n(f.seeds, kag::N_CROPS, account.seeds.begin());
    for (int u = 0; u < f.n_units; ++u)
        for (int k = 0; k < f.inv_nkeys[u]; ++k) {
            const int item = f.inv_keys[u][k];
            resources.add(u, item, f.inv[u][item]);
        }
    return true;
}
}
