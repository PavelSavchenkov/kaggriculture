#pragma once
#include "fast_game_engine/sim.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace schedule_search {
using namespace kag;
namespace fs = std::filesystem;

struct Turn { Action actions[2]; };
struct Truth {
    double cash[2];
    int inventory[N_PRODUCTS];
    uint64_t hash;
};
struct GameCase {
    Config config;
    std::vector<Turn> turns;
    std::vector<Truth> truth;
};

inline GameCase load_case(const fs::path& path) {
    std::ifstream in(path);
    GameCase result;
    auto& c = result.config;
    int count;
    std::string tag, version, sha;
    in >> c.seed >> count >> tag;
    if (!in || count != 719 || tag != "CONFIG") throw std::runtime_error("invalid case header");
    in >> c.episode_steps >> c.board_size >> c.starting_money >> c.max_orders >> c.turns_per_day
       >> c.shed_capacity >> c.weed_chance >> c.shop_unlock_interval >> c.shop_sell_interval
       >> c.center_sell_interval >> c.hire_mult;
    in >> tag >> version >> sha;
    if (tag != "ENGINE" || version != OFFICIAL_VERSION || sha != OFFICIAL_SOURCE_SHA256)
        throw std::runtime_error("case engine mismatch");
    result.turns.resize(count);
    for (auto& turn : result.turns) for (auto& action : turn.actions) {
        in >> action.n_units >> action.n_orders;
        if (!in || action.n_units < 1 || action.n_units > MAX_UNITS || action.n_orders < 0 || action.n_orders > 10)
            throw std::runtime_error("unsupported case action dimensions");
        for (int u = 0; u < action.n_units; ++u) {
            int op, arg;
            in >> op >> arg >> action.units[u].n;
            action.units[u].op = op; action.units[u].arg = arg;
        }
        for (int slot = 0; slot < action.n_orders; ++slot) {
            int op, item;
            in >> op >> item >> action.orders[slot].n;
            action.orders[slot].op = op; action.orders[slot].item = item;
        }
        action.finalize();
    }
    in >> tag;
    if (tag != "TRUTH") throw std::runtime_error("missing case truth");
    result.truth.resize(count + 1);
    for (auto& frame : result.truth) {
        in >> frame.cash[0] >> frame.cash[1];
        for (int& value : frame.inventory) in >> value;
        in >> frame.hash;
    }
    if (!in || in >> tag) throw std::runtime_error("invalid case tail");
    return result;
}

inline std::vector<Sim> validate_case(const GameCase& game) {
    Sim sim(game.config);
    std::vector<Sim> states;
    states.reserve(game.truth.size());
    for (int step = 0; step < int(game.truth.size()); ++step) {
        auto canonical = sim;
        for (auto& farm : canonical.st.farms) for (int u = 0; u < farm.n_units; ++u)
            std::sort(farm.inv_keys[u], farm.inv_keys[u] + farm.inv_nkeys[u]);
        const auto& truth = game.truth[step];
        if (canonical.parity_hash() != truth.hash || sim.reward(0) != truth.cash[0] || sim.reward(1) != truth.cash[1])
            throw std::runtime_error("full-state parity mismatch at step " + std::to_string(step));
        for (int item = 0; item < N_PRODUCTS; ++item) if (sim.st.market.inventory[item] != truth.inventory[item])
            throw std::runtime_error("market parity mismatch");
        states.push_back(sim);
        if (step < int(game.turns.size())) sim.step(game.turns[step].actions[0], game.turns[step].actions[1]);
    }
    return states;
}

inline void save_actions(const std::vector<Action>& actions, const fs::path& path) {
    std::ofstream out(path);
    out << actions.size() << '\n';
    for (const auto& a : actions) {
        out << a.n_units << ' ' << a.n_orders;
        for (int u = 0; u < a.n_units; ++u)
            out << ' ' << int(a.units[u].op) << ' ' << int(a.units[u].arg) << ' ' << a.units[u].n;
        for (int k = 0; k < a.n_orders; ++k)
            out << ' ' << int(a.orders[k].op) << ' ' << int(a.orders[k].item) << ' ' << a.orders[k].n;
        out << '\n';
    }
    if (!out) throw std::runtime_error("cannot save actions");
}

inline void pin_shops(Sim& sim, const Sim& original) {
    if (sim.st.n_shops != original.st.n_shops) throw std::runtime_error("shop unlock count changed");
    std::copy_n(original.st.shops, original.st.n_shops, sim.st.shops);
}

inline Action accepted_action(const GameCase& game, const std::vector<Sim>& states, int step, int seat) {
    Action result = game.turns[step].actions[seat];
    auto phase = [&](int slots) {
        auto sim = states[step];
        if (sim.st.hour == 23) sim.st.hour = 0;
        auto pair = game.turns[step];
        for (auto& a : pair.actions) a.n_orders = std::min(a.n_orders, slots);
        sim.step(pair.actions[0], pair.actions[1]);
        return sim;
    };
    auto previous = phase(0);
    for (int slot = 0; slot < result.n_orders; ++slot) {
        auto next = phase(slot + 1);
        const auto& a = previous.st.farms[seat];
        const auto& b = next.st.farms[seat];
        auto& order = result.orders[slot];
        int quantity = 0;
        if (order.op == M_HIRE) quantity = b.n_units - a.n_units;
        if (order.op == M_BUY_LAND) quantity = b.n_quadrants - a.n_quadrants;
        if (order.op == M_BUY_SEED && is_crop(order.item)) quantity = b.seeds[order.item] - a.seeds[order.item];
        if ((order.op == M_BUY_PRODUCT || order.op == M_BUY_ANIMAL) && order.item < N_ITEMS)
            quantity = b.shed[order.item] - a.shed[order.item];
        if (order.op == M_SELL && is_product(order.item)) quantity = b.sold_units[order.item] - a.sold_units[order.item];
        if (quantity > 0) order.n = quantity; else order = {};
        previous = std::move(next);
    }
    result.finalize();
    return result;
}
}
