#pragma once
// Shared trace reader copied from the previously verified ordinary-day extractor.
#include "extract_contract.hpp"
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace kag;
using namespace day_solver;
using namespace labor::offline;

struct Frame {
    double money[2];
    std::array<int, N_PRODUCTS> inventory;
    uint64_t hash;
};
struct Turn { Action a[2]; };
struct Trace { Config config; std::vector<Turn> turns; std::vector<Frame> truth; };

Trace load(const fs::path& path) {
    std::ifstream in(path); Trace trace; int turns; std::string tag, version, sha;
    auto& c = trace.config;
    in >> c.seed >> turns >> tag;
    if (!in || tag != "CONFIG" || turns != 719) throw std::runtime_error("invalid trace header");
    in >> c.episode_steps >> c.board_size >> c.starting_money >> c.max_orders >> c.turns_per_day >> c.shed_capacity
       >> c.weed_chance >> c.shop_unlock_interval >> c.shop_sell_interval >> c.center_sell_interval >> c.hire_mult;
    in >> tag >> version >> sha;
    if (tag != "ENGINE" || version != OFFICIAL_VERSION || sha != OFFICIAL_SOURCE_SHA256) throw std::runtime_error("engine mismatch");
    trace.turns.resize(turns);
    for (auto& turn : trace.turns) for (auto& a : turn.a) {
        in >> a.n_units >> a.n_orders;
        if (!in || a.n_units < 1 || a.n_units > MAX_UNITS || a.n_orders < 0 || a.n_orders > 10)
            throw std::runtime_error("unsupported action dimensions");
        for (int u = 0; u < a.n_units; ++u) { int op, arg; in >> op >> arg >> a.units[u].n; a.units[u].op = op; a.units[u].arg = arg; }
        for (int s = 0; s < a.n_orders; ++s) { int op, item; in >> op >> item >> a.orders[s].n; a.orders[s].op = op; a.orders[s].item = item; }
        a.finalize();
    }
    in >> tag;
    if (tag != "TRUTH") throw std::runtime_error("missing truth");
    trace.truth.resize(turns + 1);
    for (auto& f : trace.truth) {
        in >> f.money[0] >> f.money[1];
        for (auto& v : f.inventory) in >> v;
        in >> f.hash;
    }
    if (!in || in >> tag) throw std::runtime_error("wrong trace length");
    return trace;
}

void validate(const Trace& trace) {
    Sim sim(trace.config);
    for (int step = 0; step <= int(trace.turns.size()); ++step) {
        const auto& f = trace.truth[step];
        auto observed = sim;
        for (auto& farm : observed.st.farms) for (int u = 0; u < farm.n_units; ++u)
            std::sort(farm.inv_keys[u], farm.inv_keys[u] + farm.inv_nkeys[u]);
        if (observed.parity_hash() != f.hash || sim.st.farms[0].money != f.money[0] || sim.st.farms[1].money != f.money[1])
            throw std::runtime_error("full-state parity mismatch at step " + std::to_string(step));
        for (int i = 0; i < N_PRODUCTS; ++i) if (sim.st.market.inventory[i] != f.inventory[i])
            throw std::runtime_error("market parity mismatch");
        if (step < int(trace.turns.size())) sim.step(trace.turns[step].a[0], trace.turns[step].a[1]);
    }
}

std::array<Action, 24> physical(const RecordedDay& day) {
    auto result = day.own;
    for (auto& a : result) { a.n_orders = 0; std::fill(std::begin(a.orders), std::end(a.orders), Order{}); }
    for (const auto& e : day.problem.market_plan) {
        auto& a = result[e.hour]; a.n_orders = std::max(a.n_orders, int(e.order_index) + 1);
        a.orders[e.order_index] = {e.market_op, uint8_t(std::max(0, int(e.item))), e.quantity};
    }
    for (auto& a : result) a.finalize();
    return result;
}

