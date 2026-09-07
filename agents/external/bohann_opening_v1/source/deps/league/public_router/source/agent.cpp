#include "agent.hpp"
#include <array>
#include <algorithm>

namespace kag::agents::bohann_opening_v1::detail::public_router {
namespace {
using namespace kag;
#include "routes.inc"

struct Tables {
    Action actions[4][720]{};
    int future[4][721][N_PRODUCTS]{};
    Tables() {
        for (int route = 0; route < 4; ++route) {
            for (int step = 0; step < 720; ++step) {
                auto& a = actions[route][step];
                int cursor = route_offsets[route][step];
                a.n_units = route_data[cursor++];
                a.n_orders = route_data[cursor++];
                if (a.n_units > MAX_UNITS || a.n_orders > 10) std::abort();
                for (int u = 0; u < a.n_units; ++u) {
                    a.units[u] = {uint8_t(route_data[cursor]), uint8_t(route_data[cursor+1]), route_data[cursor+2]};
                    cursor += 3;
                }
                for (int o = 0; o < a.n_orders; ++o) {
                    a.orders[o] = {uint8_t(route_data[cursor]), uint8_t(route_data[cursor+1]), route_data[cursor+2]};
                    cursor += 3;
                }
                a.finalize();
            }
            for (int step = 719; step >= 0; --step) {
                std::copy_n(future[route][step+1], N_PRODUCTS, future[route][step]);
                const auto& a = actions[route][step];
                for (int o = 0; o < a.n_orders; ++o)
                    if (a.orders[o].op == M_SELL && a.orders[o].item < N_PRODUCTS)
                        future[route][step][a.orders[o].item] += a.orders[o].n;
            }
        }
    }
};

const Tables& tables() { static const Tables value; return value; }

// The donor calls this only on weeds. Preserve its deliberately narrow repair.
bool noop_on_weed(const UnitAction& a, int x, int y,
                  const agent::PrivateFarm& own, int unit) {
    switch (a.op) {
        case OP_NORTH: return y == 0;
        case OP_SOUTH: return y == BOARD-1;
        case OP_EAST: return x == BOARD-1;
        case OP_WEST: return x == 0;
        case OP_PASS: return true;
        case OP_DROP: return !is_shed_adjacent(x,y,BOARD) || own.inv_nkeys[unit] == 0;
        case OP_PICKUP: return !is_shed_adjacent(x,y,BOARD);
        case OP_PLACE: return !is_shed_adjacent(x,y,BOARD) || own.inv[unit][a.arg] <= 0;
        case OP_DIG: return false;
        default: return true;
    }
}
}

const kag::Action& Agent::planned_action(int step) const {
    return tables().actions[route_][step];
}

void Agent::act(const kag::agent::AgentObservation& obs,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    using namespace kag;
    const auto& data = tables();
    const int step = obs.step;
    if (step < 0 || step >= 720) std::abort();
    int target = -1;
    if (step == 226 && std::count(obs.shops, obs.shops+obs.n_shops, SHOP_YARN_STORE) >= 1) target = 1;
    if (step == 360 && obs.market.prices[CARROT] >= 42) target = 2;
    if (step == 433 && obs.market.inventory[MILK] >= 10067) target = 3;
    if (target >= 0 && target != route_ && route_prefix[route_][target] >= step) route_ = target;
    action = data.actions[route_][step];
    const auto& farm = obs.self();
    const int tape_units = action.n_units;
    action.n_units = farm.n_units;
    for (int u = tape_units; u < action.n_units; ++u) action.units[u] = {};
    for (int u = 0; u < std::min(tape_units, action.n_units); ++u) {
        const int x = farm.pos_x[u], y = farm.pos_y[u];
        if (farm.tiles[y][x].kind == T_WEED && noop_on_weed(action.units[u], x,y,obs.own,u))
            action.units[u] = {OP_DIG, 0, 1};
    }
    int projected[N_ITEMS];
    std::copy_n(obs.own.shed, N_ITEMS, projected);
    int room = 100 - obs.own.shed_total;
    for (int u = 0; u < std::min(tape_units, action.n_units) && room > 0; ++u) {
        if (!is_shed_adjacent(farm.pos_x[u],farm.pos_y[u],BOARD)) continue;
        const auto& a = action.units[u];
        if (a.op == OP_DROP) {
            for (int k = 0; k < obs.own.inv_nkeys[u]; ++k) {
                int item = obs.own.inv_keys[u][k];
                int take = std::min<int>(obs.own.inv[u][item], room);
                if (take > 0) { projected[item] += take; room -= take; }
            }
        } else if (a.op == OP_PLACE && !is_animal(a.arg)) {
            int take = std::min({a.n, int(obs.own.inv[u][a.arg]), room});
            if (take > 0) { projected[a.arg] += take; room -= take; }
        }
    }
    int available[N_ITEMS], planned[N_ITEMS]{};
    std::copy_n(projected, N_ITEMS, available);
    int kept = 0;
    for (int o = 0; o < action.n_orders; ++o) {
        auto order = action.orders[o];
        if (order.op == M_SELL) {
            order.n = std::min(order.n, available[order.item]);
            if (order.n <= 0) continue;
            available[order.item] -= order.n;
            planned[order.item] += order.n;
        }
        action.orders[kept++] = order;
    }
    action.n_orders = kept;
    std::array<Order, N_PRODUCTS> extras{};
    int n_extra = 0;
    for (int item = 0; item < N_PRODUCTS; ++item) {
        int have = projected[item] - planned[item];
        if (have <= 0) continue;
        int surplus = have - (obs.day >= 29 ? 0 : data.future[route_][step+1][item]);
        if (surplus > 0 && obs.market.prices[item] > 1)
            extras[n_extra++] = {M_SELL, uint8_t(item), surplus};
    }
    // Stable tie order is PRODUCTS order in Python.
    std::sort(extras.begin(), extras.begin()+n_extra, [&](const Order& a, const Order& b) {
        const int va = obs.market.prices[a.item]*a.n, vb = obs.market.prices[b.item]*b.n;
        return va != vb ? va > vb : a.item < b.item;
    });
    for (int i = 0; i < n_extra && action.n_orders < 10; ++i) action.orders[action.n_orders++] = extras[i];
    action.finalize();
}
}
