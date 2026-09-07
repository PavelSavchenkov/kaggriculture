#pragma once

#include "agents/common/api/agent_api.hpp"

namespace four_shop_foundry::observation_sim {

inline kag::Config make_config(const kag::agent::AgentConfig& source) {
    kag::Config config;
    config.episode_steps = source.episode_steps;
    config.board_size = source.board_size;
    config.starting_money = source.starting_money;
    config.max_orders = source.max_orders;
    config.turns_per_day = source.turns_per_day;
    config.shed_capacity = source.shed_capacity;
    config.weed_chance = source.weed_chance;
    config.shop_unlock_interval = source.shop_unlock_interval;
    config.shop_sell_interval = source.shop_sell_interval;
    config.center_sell_interval = source.center_sell_interval;
    config.hire_mult = source.hire_mult;
    return config;
}

inline kag::Sim make_sim(const kag::agent::AgentConfig& config,
                         const kag::agent::AgentObservation& observation) {
    kag::Sim sim(make_config(config));
    sim.st.step = observation.step;
    sim.st.day = observation.day;
    sim.st.hour = observation.hour;
    sim.st.market = observation.market;
    sim.st.n_shops = observation.n_shops;
    for (int index = 0; index < observation.n_shops; ++index)
        sim.st.shops[index] = observation.shops[index];
    for (int player = 0; player < 2; ++player) {
        const kag::agent::PublicFarm& source = observation.farms[player];
        kag::Farm& farm = sim.st.farms[player];
        farm.money = source.money;
        farm.n_units = source.n_units;
        farm.n_quadrants = source.n_quadrants;
        farm.hires_today = source.hires_today;
        for (int unit = 0; unit < source.n_units; ++unit) {
            farm.pos_x[unit] = source.pos_x[unit];
            farm.pos_y[unit] = source.pos_y[unit];
        }
        for (int y = 0; y < config.board_size; ++y)
            for (int x = 0; x < config.board_size; ++x)
                farm.tiles[y][x] = source.tiles[y][x];
    }
    kag::Farm& own = sim.st.farms[observation.player];
    own.shed_total = observation.own.shed_total;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        own.shed[item] = observation.own.shed[item];
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        own.seeds[crop] = observation.own.seeds[crop];
    for (int unit = 0; unit < own.n_units; ++unit) {
        own.inv_nkeys[unit] = observation.own.inv_nkeys[unit];
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            own.inv[unit][item] = observation.own.inv[unit][item];
            own.inv_keys[unit][item] = observation.own.inv_keys[unit][item];
        }
    }
    return sim;
}

}
