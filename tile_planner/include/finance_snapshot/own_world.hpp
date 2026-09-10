#pragma once
#include "calendar.hpp"
#include "agents/common/runtime/observation_builder.hpp"

namespace sales_planner {
inline Account own_account(const kag::agent::AgentObservation& obs) {
    Account a;
    a.cash = obs.self().money; a.total = obs.own.shed_total; a.units = obs.self().n_units;
    a.hires = obs.self().hires_today; a.quadrants = obs.self().n_quadrants;
    std::copy_n(obs.own.shed, kag::N_ITEMS, a.stock.begin());
    std::copy_n(obs.own.seeds, kag::N_CROPS, a.seeds.begin());
    return a;
}

inline Resources own_resources(const kag::agent::AgentObservation& obs) {
    Resources r;
    for (int u = 0; u < obs.self().n_units; ++u)
        for (int k = 0; k < obs.own.inv_nkeys[u]; ++k) {
            const int item = obs.own.inv_keys[u][k];
            r.add(u, item, obs.own.inv[u][item]);
        }
    return r;
}

inline kag::Config forecast_config(const kag::agent::AgentConfig& c, uint64_t seed) {
    kag::Config result;
    result.seed = seed; result.episode_steps = c.episode_steps; result.board_size = c.board_size;
    result.starting_money = c.starting_money; result.max_orders = c.max_orders;
    result.turns_per_day = c.turns_per_day; result.shed_capacity = c.shed_capacity;
    result.weed_chance = c.weed_chance; result.shop_unlock_interval = c.shop_unlock_interval;
    result.shop_sell_interval = c.shop_sell_interval; result.center_sell_interval = c.center_sell_interval;
    result.hire_mult = c.hire_mult;
    return result;
}

// A possible own future made from legal observations and an explicit forecast
// seed. Empty-prefix stepping aligns the engine's private event clocks. Past
// sampled shops/own farm are then replaced with the currently observed values.
inline kag::Sim own_world(const kag::agent::AgentObservation& obs,
                          const kag::agent::AgentConfig& config, uint64_t forecast_seed) {
    kag::Sim sim(forecast_config(config, forecast_seed));
    kag::Action pass; pass.clear(); pass.n_units = 1; pass.finalize();
    while (sim.st.step < obs.step) sim.step(pass, pass);
    sim.st.market = obs.market; sim.st.n_shops = obs.n_shops;
    std::copy_n(obs.shops, obs.n_shops, sim.st.shops);
    auto& f = sim.st.farms[0]; f = {};
    const auto& source = obs.self();
    f.money = source.money; f.n_units = source.n_units; f.n_quadrants = source.n_quadrants;
    f.hires_today = source.hires_today;
    std::copy_n(source.pos_x, kag::MAX_UNITS, f.pos_x);
    std::copy_n(source.pos_y, kag::MAX_UNITS, f.pos_y);
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            const auto tile = f.tiles[y][x] = source.tiles[y][x];
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
            const int item = obs.own.inv_keys[u][k]; f.inv_add(u, item, obs.own.inv[u][item]);
        }
    return sim;
}
}
