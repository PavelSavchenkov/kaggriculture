#pragma once
// Inputs of the learned opponent-sales forecast (scripts/train_forecast.py), shared by the dataset extractor
// (tools_dc11/forecast_extract.cpp) and the agent: dawn scalars, then the trailing 3-day opponent flow, yesterday's
// flow and dc11's own forecast (24 x 9 each, hour-major).
#include "source/history.hpp"

namespace fcast {
using namespace dc10;
constexpr int SCALARS = 72;
constexpr int GRID = HOURS * N_PRODUCTS;
constexpr int INPUTS = SCALARS + 3 * GRID;

inline void scalars(const agent::AgentObservation& o, const History& h, float* out) {
    int k = 0;
    auto put = [&](double v) { out[k++] = float(v); };
    put(o.day / 29.0);
    put((LAST_DAY - o.day) / 29.0);
    int vis_opp[N_PRODUCTS], vis_own[N_PRODUCTS];
    visible_supply(o, vis_opp);
    std::fill_n(vis_own, N_PRODUCTS, 0);
    int animals[3]{}, crops[N_CROPS]{};
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = o.self().tiles[y][x];
            if (t.has_animal) vis_own[ANIMALS[t.what - GOOSE].product] += t.yield_units;
            else if (t.kind == T_PLANT && o.day - t.planted_day >= CROPS[t.what].first_yield_day) vis_own[t.what] += t.yield_units;
            const Tile& u = o.opponent().tiles[y][x];
            if (u.has_animal) ++animals[u.what - GOOSE];
            else if (u.kind == T_PLANT) ++crops[u.what];
        }
    for (int p = 0; p < N_PRODUCTS; ++p) put(vis_opp[p] / 20.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(vis_own[p] / 20.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(h.opponent_stock()[p] / 20.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(o.own.shed[p] / 20.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(o.market.inventory[p] / 50.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(o.market.prices[p] / 200.0);
    int shops[N_SHOPS]{};
    for (int s = 0; s < o.n_shops; ++s) ++shops[o.shops[s]];
    for (int s = 0; s < N_SHOPS; ++s) put(shops[s] / 2.0);
    put(o.opponent().n_quadrants / 4.0);
    put(o.self().n_quadrants / 4.0);
    for (int a = 0; a < 3; ++a) put(animals[a] / 10.0);
    for (int c = 0; c < N_CROPS; ++c) put(crops[c] / 20.0);
    for (int p = 0; p < N_PRODUCTS; ++p) put(o.day > 0 ? h.sell_through(o.day, p, 3, 0.6) : 0.6);
    put(o.opponent().money / 50000.0);
    while (k < SCALARS) put(0);
}

// All inputs; `base` is dc11's forecast for today.
inline void inputs(const agent::AgentObservation& o, const History& h, const double base[HOURS][N_PRODUCTS], float* out) {
    scalars(o, h, out);
    double trail[HOURS][N_PRODUCTS];
    h.expected(o.day, 3, trail);
    for (int t = 0; t < HOURS; ++t)
        for (int p = 0; p < N_PRODUCTS; ++p) {
            out[SCALARS + t * N_PRODUCTS + p] = float(trail[t][p]);
            out[SCALARS + GRID + t * N_PRODUCTS + p] = o.day > 0 ? float(h.flow_at((o.day - 1) * HOURS + t, p)) : 0.0f;
            out[SCALARS + 2 * GRID + t * N_PRODUCTS + p] = float(base[t][p]);
        }
}
}  // namespace fcast
