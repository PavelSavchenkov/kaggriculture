#pragma once

#include <cstdlib>

#include "agents/common/api/agent_api.hpp"

namespace kag::agent::runtime {

inline AgentConfig make_agent_config(const Config& source) {
    return AgentConfig{
        .episode_steps = source.episode_steps,
        .board_size = source.board_size,
        .starting_money = source.starting_money,
        .max_orders = source.max_orders,
        .turns_per_day = source.turns_per_day,
        .shed_capacity = source.shed_capacity,
        .weed_chance = source.weed_chance,
        .shop_unlock_interval = source.shop_unlock_interval,
        .shop_sell_interval = source.shop_sell_interval,
        .center_sell_interval = source.center_sell_interval,
        .hire_mult = source.hire_mult,
    };
}

inline AgentInit make_agent_init(const Sim& sim, int player) {
    if (player < 0 || player > 1) std::abort();
    return AgentInit{make_agent_config(sim.cfg), static_cast<uint8_t>(player)};
}

inline AgentObservation make_observation(const Sim& sim, int player) {
    if (player < 0 || player > 1 || sim.st.n_shops < 0 ||
        sim.st.n_shops > MAX_SHOP_INSTANCES)
        std::abort();

    AgentObservation out{};
    out.player = static_cast<uint8_t>(player);
    out.step = sim.st.step;
    out.day = sim.st.day;
    out.hour = sim.st.hour;
    out.market = sim.st.market;
    out.n_shops = sim.st.n_shops;
    for (int shop = 0; shop < out.n_shops; ++shop)
        out.shops[shop] = sim.st.shops[shop];

    for (int seat = 0; seat < 2; ++seat) {
        const Farm& source = sim.st.farms[seat];
        if (source.n_units < 1 || source.n_units > MAX_UNITS ||
            source.n_quadrants < 1 || source.n_quadrants > 4)
            std::abort();

        PublicFarm& target = out.farms[seat];
        target.money = source.money;
        target.n_units = source.n_units;
        target.n_quadrants = source.n_quadrants;
        target.hires_today = source.hires_today;
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x)
                target.tiles[y][x] = source.tiles[y][x];
        for (int unit = 0; unit < source.n_units; ++unit) {
            target.pos_x[unit] = source.pos_x[unit];
            target.pos_y[unit] = source.pos_y[unit];
        }
    }

    const Farm& source = sim.st.farms[player];
    PrivateFarm& target = out.own;
    target.shed_total = source.shed_total;
    for (int item = 0; item < N_ITEMS; ++item)
        target.shed[item] = source.shed[item];
    for (int crop = 0; crop < N_CROPS; ++crop)
        target.seeds[crop] = source.seeds[crop];
    for (int unit = 0; unit < source.n_units; ++unit) {
        if (source.inv_nkeys[unit] > N_ITEMS) std::abort();
        target.inv_nkeys[unit] = source.inv_nkeys[unit];
        for (int item = 0; item < N_ITEMS; ++item)
            target.inv[unit][item] = source.inv[unit][item];
        for (int key = 0; key < source.inv_nkeys[unit]; ++key)
            target.inv_keys[unit][key] = source.inv_keys[unit][key];
    }
    return out;
}

}
