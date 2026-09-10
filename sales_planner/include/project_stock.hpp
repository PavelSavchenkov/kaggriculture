#pragma once
#include "market.hpp"
#include "agents/common/api/agent_api.hpp"

namespace sales_planner {
// Exact shed projection for the supported action patterns. Animal placement on
// a shed-access tile can depend on earlier same-turn construction; the overlay
// leaves that rare pattern unchanged until a complete tile projection is added.
inline bool project_stock(const kag::agent::AgentObservation& o, const kag::Action& action,
                          Account& out, int capacity = 100) {
    out = {};
    out.cash = o.self().money; out.units = o.self().n_units;
    out.hires = o.self().hires_today; out.quadrants = o.self().n_quadrants;
    out.total = o.own.shed_total;
    std::copy_n(o.own.shed, kag::N_ITEMS, out.stock.begin());
    std::copy_n(o.own.seeds, kag::N_CROPS, out.seeds.begin());
    for (int u = 0; u < o.self().n_units; ++u) {
        const int x = o.self().pos_x[u], y = o.self().pos_y[u];
        if ((x != 4 && x != 5) || (y != 4 && y != 5)) continue;
        const auto a = action.units[u];
        if (a.op == kag::OP_PICKUP && a.arg < kag::N_ITEMS) {
            const int n = std::min(out.stock[a.arg], std::max(0, a.n));
            out.stock[a.arg] -= n; out.total -= n;
        } else if (a.op == kag::OP_DROP) {
            for (int k = 0; k < o.own.inv_nkeys[u]; ++k) {
                const int item = o.own.inv_keys[u][k];
                const int n = std::min(int(o.own.inv[u][item]), capacity - out.total);
                out.stock[item] += n; out.total += n;
            }
        } else if (a.op == kag::OP_PLACE && a.arg < kag::N_ITEMS) {
            if (kag::is_animal(a.arg)) return false;
            const int n = std::min({std::max(0, a.n), int(o.own.inv[u][a.arg]), capacity - out.total});
            out.stock[a.arg] += n; out.total += n;
        }
    }
    return true;
}
}
