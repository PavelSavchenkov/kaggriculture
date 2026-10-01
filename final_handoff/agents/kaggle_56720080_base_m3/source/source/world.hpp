#pragma once
// Engine helpers shared by the converter, compiler and tools.
#include "agents/common/runtime/observation_builder.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dc10 {
using namespace kag;

constexpr int HOURS = 24;
constexpr int LAST_DAY = 29;
constexpr int SHED_X0 = BOARD / 2 - 1;  // shed access tiles are x,y in {4,5}

inline int cell_of(int x, int y) { return y * BOARD + x; }
inline int cell_x(int cell) { return cell % BOARD; }
inline int cell_y(int cell) { return cell / BOARD; }
inline int dist(int a, int b) { return std::abs(cell_x(a) - cell_x(b)) + std::abs(cell_y(a) - cell_y(b)); }
inline bool shed_access(int cell) { return is_shed_adjacent(cell_x(cell), cell_y(cell), BOARD); }
// Moves from a cell to the nearest shed-access tile.
inline int shed_dist(int cell) {
    int best = 99;
    for (int y = SHED_X0; y <= SHED_X0 + 1; ++y)
        for (int x = SHED_X0; x <= SHED_X0 + 1; ++x) best = std::min(best, dist(cell, cell_of(x, y)));
    return best;
}
inline int nearest_shed_cell(int cell) {
    int best = -1;
    for (int y = SHED_X0; y <= SHED_X0 + 1; ++y)
        for (int x = SHED_X0; x <= SHED_X0 + 1; ++x) {
            const int c = cell_of(x, y);
            if (best < 0 || dist(cell, c) < dist(cell, best)) best = c;
        }
    return best;
}
// Last playable hour of a day: day 29 has 23 turns.
inline int last_hour(int day) { return day == LAST_DAY ? HOURS - 2 : HOURS - 1; }

inline const Tile& tile_at(const agent::AgentObservation& o, int cell) {
    return o.self().tiles[cell_y(cell)][cell_x(cell)];
}

// A Sim that reproduces our farm, the public opponent farm and the market from an
// observation. Opponent private stock is unknown and left empty. Stepping a fresh
// Sim with PASS actions first aligns its private town/shop clocks with the step.
inline Sim sim_from_observation(const agent::AgentObservation& o, const Config& config, bool align_clock = true) {
    Sim sim(config);
    Action pass;
    pass.finalize();
    while (align_clock && sim.st.step < o.step) sim.step(pass, pass);
    State& st = sim.st;
    st.market = o.market;
    st.n_shops = o.n_shops;
    for (int s = 0; s < o.n_shops; ++s) st.shops[s] = o.shops[s];
    for (int seat = 0; seat < 2; ++seat) {
        Farm f{};
        const auto& pub = o.farms[seat];
        f.money = pub.money;
        f.n_units = pub.n_units;
        f.n_quadrants = pub.n_quadrants;
        f.hires_today = pub.hires_today;
        for (int u = 0; u < pub.n_units; ++u) f.pos_x[u] = pub.pos_x[u], f.pos_y[u] = pub.pos_y[u];
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) {
                const Tile& t = pub.tiles[y][x];
                f.tiles[y][x] = t;
                const int index = cell_of(x, y);
                const uint64_t bit = uint64_t{1} << (index & 63);
                if (t.kind == T_EMPTY) f.empty_mask[index >> 6] |= bit;
                if (t.kind == T_PLANT) f.plant_mask[index >> 6] |= bit;
                if (t.has_animal) f.animal_mask[index >> 6] |= bit;
                if (t.kind == T_PLANT && t.max_lifespan_step >= 0) f.decay_mask[index >> 6] |= bit;
            }
        f.next_decay_step = 0;
        if (seat == o.player) {
            const auto& own = o.own;
            for (int i = 0; i < N_ITEMS; ++i) f.shed[i] = own.shed[i];
            f.shed_total = own.shed_total;
            for (int c = 0; c < N_CROPS; ++c) f.seeds[c] = own.seeds[c];
            for (int u = 0; u < pub.n_units; ++u) {
                for (int i = 0; i < N_ITEMS; ++i) f.inv[u][i] = own.inv[u][i];
                f.inv_nkeys[u] = own.inv_nkeys[u];
                for (int k = 0; k < own.inv_nkeys[u]; ++k) f.inv_keys[u][k] = own.inv_keys[u][k];
            }
        }
        st.farms[seat] = f;
    }
    st.step = o.step;
    st.day = o.day;
    st.hour = o.hour;
    return sim;
}

// ---------------------------------------------------------------- replays
// Trace format: see data/FORMAT.md (engine text format with per-state truth).
struct Replay {
    Config config;
    std::vector<std::array<Action, 2>> turns;
    std::vector<uint64_t> hashes;
    std::vector<std::array<double, 2>> cash;
};

inline Replay load_replay(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path);
    Replay r;
    auto& c = r.config;
    int count = 0;
    std::string tag, version, sha;
    in >> c.seed >> count >> tag;
    if (!in || count != 719 || tag != "CONFIG") throw std::runtime_error("bad header " + path);
    in >> c.episode_steps >> c.board_size >> c.starting_money >> c.max_orders >> c.turns_per_day >>
        c.shed_capacity >> c.weed_chance >> c.shop_unlock_interval >> c.shop_sell_interval >>
        c.center_sell_interval >> c.hire_mult;
    in >> tag >> version >> sha;
    if (tag != "ENGINE" || version != OFFICIAL_VERSION || sha != OFFICIAL_SOURCE_SHA256)
        throw std::runtime_error("engine mismatch " + path);
    r.turns.resize(count);
    for (auto& turn : r.turns)
        for (auto& a : turn) {
            in >> a.n_units >> a.n_orders;
            if (!in || a.n_units < 1 || a.n_units > MAX_UNITS || a.n_orders < 0 || a.n_orders > 10)
                throw std::runtime_error("bad action " + path);
            for (int u = 0; u < a.n_units; ++u) {
                int op, arg;
                in >> op >> arg >> a.units[u].n;
                a.units[u].op = uint8_t(op);
                a.units[u].arg = uint8_t(arg);
            }
            for (int k = 0; k < a.n_orders; ++k) {
                int op, item;
                long long n = 0;  // some Kaggle agents sell with n = 2^53 - 1 ("everything"): clamp into int32
                in >> op >> item >> n;
                a.orders[k].n = int32_t(std::clamp(n, -(1LL << 24), 1LL << 24));
                a.orders[k].op = uint8_t(op);
                a.orders[k].item = uint8_t(item);
            }
            a.finalize();
        }
    in >> tag;
    if (tag != "TRUTH") throw std::runtime_error("missing truth " + path);
    r.hashes.resize(count + 1);
    r.cash.resize(count + 1);
    for (int s = 0; s <= count; ++s) {
        int inventory;
        in >> r.cash[s][0] >> r.cash[s][1];
        for (int i = 0; i < N_PRODUCTS; ++i) in >> inventory;
        in >> r.hashes[s];
    }
    if (!in) throw std::runtime_error("bad truth " + path);
    return r;
}

// Writes a trace readable by load_replay: config, both seats' actions, and the truth
// (cash, market inventory and parity hash of every state) from re-simulating them.
inline void save_replay(const std::string& path, const Config& c, const std::vector<std::array<Action, 2>>& turns) {
    std::ofstream out(path);
    out.precision(17);
    out << c.seed << ' ' << turns.size() << "\nCONFIG " << c.episode_steps << ' ' << c.board_size << ' '
        << c.starting_money << ' ' << c.max_orders << ' ' << c.turns_per_day << ' ' << c.shed_capacity << ' '
        << c.weed_chance << ' ' << c.shop_unlock_interval << ' ' << c.shop_sell_interval << ' ' << c.center_sell_interval
        << ' ' << c.hire_mult << "\nENGINE " << OFFICIAL_VERSION << ' ' << OFFICIAL_SOURCE_SHA256 << '\n';
    for (const auto& turn : turns)
        for (const Action& a : turn) {
            out << int(a.n_units) << ' ' << int(a.n_orders);
            for (int u = 0; u < a.n_units; ++u) out << ' ' << int(a.units[u].op) << ' ' << int(a.units[u].arg) << ' ' << a.units[u].n;
            for (int k = 0; k < a.n_orders; ++k) out << ' ' << int(a.orders[k].op) << ' ' << int(a.orders[k].item) << ' ' << a.orders[k].n;
            out << '\n';
        }
    out << "TRUTH\n";
    Sim sim(c);
    for (size_t s = 0; s <= turns.size(); ++s) {
        Sim canonical = sim;
        for (auto& farm : canonical.st.farms)
            for (int u = 0; u < farm.n_units; ++u) std::sort(farm.inv_keys[u], farm.inv_keys[u] + farm.inv_nkeys[u]);
        out << sim.st.farms[0].money << ' ' << sim.st.farms[1].money;
        for (int i = 0; i < N_PRODUCTS; ++i) out << ' ' << sim.st.market.inventory[i];
        out << ' ' << canonical.parity_hash() << '\n';
        if (s < turns.size()) sim.step(turns[s][0], turns[s][1]);
    }
}

// All 720 states of a replay, each checked against the recorded parity hash.
inline std::vector<Sim> replay_states(const Replay& r) {
    Sim sim(r.config);
    std::vector<Sim> states;
    states.reserve(r.hashes.size());
    for (size_t s = 0; s < r.hashes.size(); ++s) {
        Sim canonical = sim;
        for (auto& farm : canonical.st.farms)
            for (int u = 0; u < farm.n_units; ++u)
                std::sort(farm.inv_keys[u], farm.inv_keys[u] + farm.inv_nkeys[u]);
        if (canonical.parity_hash() != r.hashes[s]) throw std::runtime_error("replay parity mismatch");
        states.push_back(sim);
        if (s < r.turns.size()) sim.step(r.turns[s][0], r.turns[s][1]);
    }
    return states;
}

// Step a copy without the day-end update, exposing today's service flags.
inline Sim step_without_night(const Sim& sim, const Action& a0, const Action& a1) {
    Sim copy = sim;
    copy.cfg.turns_per_day = 1 << 20;
    copy.step(a0, a1);
    return copy;
}
}
