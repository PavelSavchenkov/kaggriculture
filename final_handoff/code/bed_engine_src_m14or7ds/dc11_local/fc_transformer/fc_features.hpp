#pragma once
// Features of the learned opponent-sales forecast (fc_model.hpp) at a dawn, in the order of fc_features.py F, built
// from what the agent has: the dawn observation, its History (opponent sales inferred per step; use the hour-23 shed fix)
// and the seller's own forecast for today. Mirrors probe/fc_rows.cpp + fc_features.py prepare(); the game-history
// features (since, cum, cs0-3, cdays, mean_first_hour) come from History's flows here (training used the recorded
// sales; equal except $1-floor sales).
// Use on days 1-28 only (the training range); keep the seller's own forecast on day 0 and day 29. Previous-day reads are
// guarded anyway (History::flow_at does not check negative steps).
#include "source/history.hpp"
#include <array>

namespace fcmodel {
constexpr int N_FEATURES = 34;

inline std::array<double, N_FEATURES> features(const dc10::agent::AgentObservation& o, const dc10::History& h,
                                               const double forecast[dc10::HOURS][dc10::N_PRODUCTS], int p) {
    using namespace dc10;
    const int d = o.day;
    // opponent's past sales of p by day and hour (inferred), up to yesterday
    int since = 99, cum = 0, cdays = 0, first_sum = 0, first_n = 0;
    double band_cum[4]{};
    for (int day = 0; day < d; ++day) {
        int total = 0, first = -1;
        for (int hour = 0; hour < HOURS; ++hour) {
            const int u = std::max(0, h.flow_at(day * HOURS + hour, p));
            total += u, band_cum[hour / 6] += u;
            if (u > 0 && first < 0) first = hour;
        }
        if (total > 0) since = d - day, ++cdays, first_sum += first, ++first_n;
        cum += total;
    }
    int yb[4]{};
    if (d >= 1)
        for (int hour = 0; hour < HOURS; ++hour) yb[hour / 6] += h.flow_at((d - 1) * HOURS + hour, p);
    int vis[N_PRODUCTS];
    visible_supply(o, vis);
    int herd = 0, ready = 0;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = o.opponent().tiles[y][x];
            if (t.has_animal && ANIMALS[t.what - GOOSE].product == p) ++herd, ready += t.yield_units > 0;
        }
    int demand = 0;
    for (int k = 0; k < HOURS; ++k) {
        const int step = o.step + k;
        if (step % 4 == 0)
            for (int s = 0; s < o.n_shops; ++s)
                if (SHOP_MASK[o.shops[s]] & (1u << p)) demand += SHOP_MULT[o.shops[s]];
        if (step % HOURS == 0) demand += 1;
    }
    double ft = 0, fb[4]{};
    for (int hour = 0; hour < HOURS; ++hour) {
        const double v = std::max(0.0, forecast[hour][p]);
        ft += v, fb[hour / 6] += v;
    }
    const double band_total = band_cum[0] + band_cum[1] + band_cum[2] + band_cum[3];
    const double stock = h.opponent_stock()[p], through = h.sell_through(d, p, 3, 0.6);
    std::array<double, N_FEATURES> f{
        double(d), double(d >= 1 ? h.sold_on(d - 1, p) : 0), double(d >= 2 ? h.sold_on(d - 2, p) : 0), double(d >= 3 ? h.sold_on(d - 3, p) : 0),
        double(yb[0]), double(yb[1]), double(yb[2]), double(yb[3]), double(since), double(cum), stock, double(vis[p]),
        stock + vis[p], through * (stock + vis[p]), through, double(herd), double(ready),
        double(o.market.inventory[p] - 10000), double(market_price(p, o.market.inventory[p])), double(demand),
        double(o.own.shed[p]), o.self().money, o.opponent().money, ft,
        ft > 0 ? fb[0] / ft : 0.0, ft > 0 ? fb[1] / ft : 0.0, ft > 0 ? fb[2] / ft : 0.0, ft > 0 ? fb[3] / ft : 0.0,
        band_total > 0 ? band_cum[0] / band_total : -1.0, band_total > 0 ? band_cum[1] / band_total : -1.0,
        band_total > 0 ? band_cum[2] / band_total : -1.0, band_total > 0 ? band_cum[3] / band_total : -1.0,
        double(cdays), first_n ? double(first_sum) / first_n : -1.0};
    return f;
}
}
