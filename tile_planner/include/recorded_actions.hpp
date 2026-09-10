#pragma once
#include "course.hpp"

namespace placement {
// Recorded rival commands may contain invalid item arguments that the engine
// treats as no-ops. Keep them for exact engine replay; own certificates still
// use the stricter public schedule reader.
inline Schedule read_recorded_actions(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read recorded actions");
    Schedule result;
    for (auto& action : result) {
        input >> action.n_units >> action.n_orders;
        if (!input || action.n_units < 1 || action.n_units > kag::MAX_UNITS || action.n_orders < 0 || action.n_orders > 10)
            throw std::runtime_error("invalid recorded action dimensions");
        for (int u = 0; u < action.n_units; ++u) {
            int op = -1, item = -1; input >> op >> item >> action.units[u].n;
            if (!input || op < 0 || op > kag::OP_INVALID || item < 0 || item > 255)
                throw std::runtime_error("invalid recorded unit encoding");
            action.units[u].op = op; action.units[u].arg = item;
        }
        for (int s = 0; s < action.n_orders; ++s) {
            int op = -1, item = -1; input >> op >> item >> action.orders[s].n;
            if (!input || op < 0 || op > kag::M_SELL || item < 0 || item >= kag::N_ITEMS)
                throw std::runtime_error("unsupported recorded market encoding");
            action.orders[s].op = op; action.orders[s].item = item;
        }
        action.finalize();
    }
    std::string extra;
    if (!input || input >> extra) throw std::runtime_error("invalid recorded action length");
    return result;
}
}
