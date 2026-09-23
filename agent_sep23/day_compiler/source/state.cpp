#include "state.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>

namespace kag::day_compiler {
int distance(int first, int second) {
    return std::abs(first / BOARD - second / BOARD) + std::abs(first % BOARD - second % BOARD);
}
int shed_distance(int cell) {
    const int x = cell % BOARD, y = cell / BOARD;
    return std::max({4 - x, x - 5, 0}) + std::max({4 - y, y - 5, 0});
}
int demand(const Observation& o, const Configuration& c, int product, int step) {
    int count = product < FERTILIZER && step % c.center_sell_interval == 0;
    if (step % c.shop_sell_interval == 0)
        for (int shop = 0; shop < o.n_shops; ++shop)
            if (SHOP_MASK[o.shops[shop]] & (1u << product)) count += SHOP_MULT[o.shops[shop]];
    return count;
}
void rebuild_masks(Farm& farm) {
    for (auto* mask : {farm.empty_mask, farm.plant_mask, farm.animal_mask, farm.decay_mask})
        std::fill_n(mask, 2, 0);
    farm.next_decay_step = INT_MAX;
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        const auto& tile = farm.tiles[cell / BOARD][cell % BOARD];
        const auto bit = uint64_t{1} << (cell % 64); const int word = cell / 64;
        if (tile.kind == T_EMPTY) farm.empty_mask[word] |= bit;
        if (tile.has_animal) farm.animal_mask[word] |= bit;
        if (tile.kind != T_PLANT) continue;
        farm.plant_mask[word] |= bit;
        if (tile.max_lifespan_step >= 0) {
            farm.decay_mask[word] |= bit;
            farm.next_decay_step = std::min(farm.next_decay_step, tile.max_lifespan_step);
        }
    }
}
namespace {
Farm public_farm(const agent::PublicFarm& visible) {
    Farm farm{};
    farm.money = visible.money; farm.n_units = visible.n_units;
    farm.n_quadrants = visible.n_quadrants; farm.hires_today = visible.hires_today;
    for (int row = 0; row < BOARD; ++row) std::copy_n(visible.tiles[row], BOARD, farm.tiles[row]);
    std::copy_n(visible.pos_x, MAX_UNITS, farm.pos_x);
    std::copy_n(visible.pos_y, MAX_UNITS, farm.pos_y);
    rebuild_masks(farm);
    return farm;
}
}
Farm own_farm(const Observation& o) {
    auto farm = public_farm(o.self());
    std::copy_n(o.own.shed, N_ITEMS, farm.shed); farm.shed_total = o.own.shed_total;
    std::copy_n(o.own.seeds, N_CROPS, farm.seeds);
    for (int unit = 0; unit < farm.n_units; ++unit) {
        std::copy_n(o.own.inv[unit], N_ITEMS, farm.inv[unit]);
        farm.inv_nkeys[unit] = o.own.inv_nkeys[unit];
        std::copy_n(o.own.inv_keys[unit], farm.inv_nkeys[unit], farm.inv_keys[unit]);
    }
    return farm;
}
Sim observed_state(const Observation& o, const Configuration& c) {
    Config config;
    config.seed = 0; config.weed_chance = 0; config.starting_money = c.starting_money;
    config.episode_steps = c.episode_steps; config.board_size = c.board_size;
    config.max_orders = c.max_orders; config.turns_per_day = c.turns_per_day;
    config.shed_capacity = c.shed_capacity; config.hire_mult = c.hire_mult;
    config.shop_sell_interval = c.shop_sell_interval; config.center_sell_interval = c.center_sell_interval;
    config.shop_unlock_interval = c.episode_steps + 1;
    Sim sim(config);
    // Initialize the engine's private recovery clocks without hidden state.
    while (sim.st.step < o.step) sim.step(Action{}, Action{});
    sim.st.day = o.day; sim.st.hour = o.hour; sim.st.done = false;
    sim.st.market = o.market;
    sim.st.n_shops = o.n_shops; std::copy_n(o.shops, o.n_shops, sim.st.shops);
    sim.st.farms[o.player] = own_farm(o);
    sim.st.farms[o.player ^ 1] = public_farm(o.opponent());
    return sim;
}
Farm worker_phase(const Observation& o, const Action& action, const Configuration& c) {
    Config config; config.shed_capacity = c.shed_capacity;
    Sim sim(config);
    sim.st.day = o.day; sim.st.hour = 0; sim.st.step = -100000;
    sim.st.farms[o.player] = own_farm(o);
    Action workers = action; workers.n_orders = 0; workers.finalize();
    if (o.player == 0) sim.step(workers, Action{}); else sim.step(Action{}, workers);
    return sim.st.farms[o.player];
}
bool same_entity(const Tile& a, const Tile& b) {
    return a.kind == b.kind && a.what == b.what && a.has_animal == b.has_animal && a.planted_day == b.planted_day;
}
}
