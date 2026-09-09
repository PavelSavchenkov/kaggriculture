#pragma once
#include <day_solver/scheduler.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

namespace labor::offline {
using Schedule = std::array<kag::Action, 24>;

inline Schedule read_actions(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read actions: " + path);
    Schedule actions;
    for (auto& a : actions) {
        input >> a.n_units >> a.n_orders;
        if (!input || a.n_units < 1 || a.n_units > kag::MAX_UNITS || a.n_orders < 0 || a.n_orders > 10)
            throw std::runtime_error("invalid action dimensions");
        for (int u = 0; u < a.n_units; ++u) {
            int op, arg; input >> op >> arg >> a.units[u].n;
            if (op < 0 || op > kag::OP_CARE || arg < 0 || arg >= kag::N_ITEMS)
                throw std::runtime_error("invalid unit action");
            a.units[u].op = op; a.units[u].arg = arg;
        }
        for (int s = 0; s < a.n_orders; ++s) {
            int op, item; input >> op >> item >> a.orders[s].n;
            if (op < 0 || op > kag::M_SELL || item < 0 || item >= kag::N_ITEMS)
                throw std::runtime_error("invalid market order");
            a.orders[s].op = op; a.orders[s].item = item;
        }
        a.finalize();
    }
    std::string extra;
    if (!input || input >> extra) throw std::runtime_error("invalid action file length");
    return actions;
}

inline void physical_orders(const day_solver::DayProblem& problem, Schedule& actions) {
    for (auto& a : actions) {
        a.n_orders = 0;
        std::fill(std::begin(a.orders), std::end(a.orders), kag::Order{});
    }
    for (const auto& e : problem.market_plan) {
        auto& a = actions[e.hour];
        a.n_orders = std::max(a.n_orders, int(e.order_index) + 1);
        a.orders[e.order_index] = {e.market_op, uint8_t(std::max(0, int(e.item))), e.quantity};
    }
    for (auto& a : actions) a.finalize();
}

inline void save_actions(const Schedule& actions, const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot save schedule");
    for (const auto& a : actions) {
        out << a.n_units << ' ' << a.n_orders;
        for (int u = 0; u < a.n_units; ++u) out << ' ' << +a.units[u].op << ' ' << +a.units[u].arg << ' ' << a.units[u].n;
        for (int s = 0; s < a.n_orders; ++s) out << ' ' << +a.orders[s].op << ' ' << +a.orders[s].item << ' ' << a.orders[s].n;
        out << '\n';
    }
}
}
