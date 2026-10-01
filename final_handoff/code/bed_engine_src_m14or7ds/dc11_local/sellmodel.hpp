#pragma once
// Learned M&M seller (I1, BC, Sep 30): per hour and product, a distribution over the units M&M sells (MLP trained on M&M's recorded
// games by sep29_bc_mm/scripts/i1_train.py, text weights <model>.sellmodel). Execution SAMPLES from it: the mode under-sells by ~half
// (M&M sells a product in only 4-13% of hours, in lots of 30-50% of its stock right after the shop drains).
// Features (i1_train.py features(), same order and scaling): hour one-hot 24, hour % 4 one-hot 4, day / 29, log1p(money) / 12,
// log1p(opponent money) / 12, units / 15, quadrants / 4; per product: shed, pocket, field (harvestable held yield), due (productions
// tonight), market inventory, price, shops buying it, opponent stock, opponent sales over the last 1 / 4 / 24 hours, own units sold
// today; product one-hot. Optional inputs, by the model's input count (i1_train.py features=...):
//   cap (56): two globals after the 33, total shed stock / 100 and total pocket stock / 100.
//   level (63): globals 33-37 = day 25..29 one-hot; per product + market inventory and price minus their means over the previous 4 / 24
//     hours (/ 50 and / base price). Needs a Tracker updated every hour.
//   level+cap (65): both (the cap globals after the level ones).
#include "source/history.hpp"
#include <array>
#include <cmath>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace bcsell {

using namespace kag;
constexpr int P = N_PRODUCTS;
constexpr int GLOBAL = 33, PER = 12, IN = GLOBAL + PER + P, IN_CAP = IN + 2, IN_LEVEL = IN + 5 + 4, IN_LEVEL_CAP = IN_LEVEL + 2;
inline bool has_level(int in) { return in == IN_LEVEL || in == IN_LEVEL_CAP; }
inline bool has_cap(int in) { return in == IN_CAP || in == IN_LEVEL_CAP; }
constexpr double BASE[P] = {25, 35, 60, 120, 250, 50, 160, 200, 100};

struct Net {
    int in = 0, width = 0, k = 0;
    std::vector<float> w1, b1, w2, b2, w3, b3;
};

// One or more networks (text blocks "sellmodel 1 <in> <width> <k>" + 6 weight lines each); probabilities are averaged over them.
struct Model {
    std::vector<Net> nets;
    int k = 0, in = 0;
    // Optional "thresholds t0 .. t8" line (scripts/i1_decode.py): sell product p when P(units > 0) > t_p, which gives M&M's share of
    // selling hours on the training games, and sell the expected lot given a sale (rounded). Deterministic; aligns lots with M&M's hours better
    // than sampling (held-out overlap 0.52-0.76 vs 0.38-0.68).
    std::vector<double> thresholds;
    bool load(const std::string& path) {
        std::FILE* f = std::fopen(path.c_str(), "r");
        if (!f) return false;
        bool ok = true;
        for (int version = 0;;) {
            Net n;
            const int got = std::fscanf(f, " sellmodel %d %d %d %d", &version, &n.in, &n.width, &n.k);
            if (got == EOF || got == 0) {
                double t;
                if (got == 0 && std::fscanf(f, " thresholds") == 0)
                    while (std::fscanf(f, "%lf", &t) == 1) thresholds.push_back(t);
                break;
            }
            ok = ok && got == 4 && version == 1 && (n.in == IN || has_cap(n.in) || has_level(n.in)) &&
                 (nets.empty() || (n.k == nets[0].k && n.in == nets[0].in));
            auto read = [&](std::vector<float>& v, size_t count) {
                v.resize(count);
                for (float& x : v) ok = ok && std::fscanf(f, "%f", &x) == 1;
            };
            if (!ok) break;
            read(n.w1, size_t(n.width) * n.in), read(n.b1, n.width), read(n.w2, size_t(n.width) * n.width), read(n.b2, n.width);
            read(n.w3, size_t(n.k + 1) * n.width), read(n.b3, n.k + 1);
            if (!ok) break;
            nets.push_back(std::move(n));
        }
        std::fclose(f);
        if (!ok || nets.empty() || !(thresholds.empty() || int(thresholds.size()) == P)) return false;
        k = nets[0].k, in = nets[0].in;
        return true;
    }
    // Probabilities of selling 0..k units for one feature vector (mean over the networks).
    void probs(const float* x, double* out) const {
        for (int c = 0; c <= k; ++c) out[c] = 0;
        std::vector<double> one(k + 1);
        for (const Net& n : nets) {
            std::vector<float> h1(n.width), h2(n.width);
            for (int i = 0; i < n.width; ++i) {
                float s = n.b1[i];
                for (int j = 0; j < n.in; ++j) s += n.w1[size_t(i) * n.in + j] * x[j];
                h1[i] = s > 0 ? s : 0;
            }
            for (int i = 0; i < n.width; ++i) {
                float s = n.b2[i];
                for (int j = 0; j < n.width; ++j) s += n.w2[size_t(i) * n.width + j] * h1[j];
                h2[i] = s > 0 ? s : 0;
            }
            double top = -1e300, z = 0;
            for (int c = 0; c <= k; ++c) {
                double s = n.b3[c];
                for (int j = 0; j < n.width; ++j) s += n.w3[size_t(c) * n.width + j] * h2[j];
                one[c] = s, top = std::max(top, s);
            }
            for (int c = 0; c <= k; ++c) z += one[c] = std::exp(one[c] - top);
            for (int c = 0; c <= k; ++c) out[c] += one[c] / z / double(nets.size());
        }
    }
};

// Raw per-hour inputs (what sell_rows writes); the obs-based builder below fills it.
struct Raw {
    int day = 0, hour = 0, units = 1, quadrants = 1, shed_total = 0, pocket_total = 0;
    double money = 0, opp_money = 0;
    int shed[P]{}, pocket[P]{}, field[P]{}, due[P]{}, inv[P]{}, shops[P]{}, opp_stock[P]{}, opp1[P]{}, opp4[P]{}, opp24[P]{}, sold_today[P]{};
    double price[P]{};
    double inv_m4[P]{}, inv_m24[P]{}, price_m4[P]{}, price_m24[P]{};  // level models: value minus its trailing mean
};

// Market inventory and price per product for every hour seen (update() once per hour, before sell_units; repeated calls in the same
// hour are ignored). levels() fills the Raw's trailing differences from the hours before the current one, as i1_train.py add_level
// (the mean of the previous min(w, hours seen) hours; 0 at the first hour).
struct Tracker {
    std::vector<std::array<double, 2 * P>> past;
    int last_step = -1;
    void update(const agent::AgentObservation& o) {
        if (o.step == last_step) return;
        if (o.step < last_step) past.clear();  // new game
        std::array<double, 2 * P> e;
        for (int p = 0; p < P; ++p) e[p] = o.market.inventory[p], e[P + p] = o.market.prices[p];
        past.push_back(e), last_step = o.step;
    }
    // r.inv / r.price must be the current hour's (the last update); the means use the entries before it.
    void levels(Raw& r) const {
        const int before = int(past.size()) - 1;
        if (before < 0) throw std::logic_error("sellmodel Tracker: update() was not called");
        for (int p = 0; p < P; ++p)
            for (int q = 0; q < 2; ++q) {
                const double v = q == 0 ? double(r.inv[p]) : r.price[p];
                for (const int w : {4, 24}) {
                    const int n = std::min(w, before);
                    double m = v;
                    if (n > 0) {
                        m = 0;
                        for (int j = 1; j <= n; ++j) m += past[before - j][q * P + p];
                        m /= n;
                    }
                    (q == 0 ? (w == 4 ? r.inv_m4 : r.inv_m24) : (w == 4 ? r.price_m4 : r.price_m24))[p] = v - m;
                }
            }
    }
};

inline void features(const Raw& r, int p, float* x, int in = IN) {
    for (int i = 0; i < in; ++i) x[i] = 0;
    x[r.hour] = 1, x[24 + r.hour % 4] = 1;
    x[28] = float(r.day / 29.0), x[29] = float(std::log1p(std::max(0.0, r.money)) / 12.0);
    x[30] = float(std::log1p(std::max(0.0, r.opp_money)) / 12.0), x[31] = float(r.units / 15.0), x[32] = float(r.quadrants / 4.0);
    const bool level = has_level(in), cap = has_cap(in);
    const int global = GLOBAL + (level ? 5 : 0) + (cap ? 2 : 0), per = PER + (level ? 4 : 0);
    if (level && r.day >= 25) x[33 + std::min(r.day - 25, 4)] = 1;
    if (cap) {
        const int c = 33 + (level ? 5 : 0);
        x[c] = float(r.shed_total / 100.0), x[c + 1] = float(r.pocket_total / 100.0);
    }
    auto lg = [](int v) { return float(std::log1p(double(std::max(0, v))) / 4.0); };
    float* f = x + global;
    f[0] = lg(r.shed[p]), f[1] = lg(r.pocket[p]), f[2] = lg(r.field[p]), f[3] = lg(r.due[p]);
    f[4] = float((r.inv[p] - 10000) / 500.0), f[5] = float(r.price[p] / BASE[p]), f[6] = float(r.shops[p] / 4.0);
    f[7] = lg(r.opp_stock[p]), f[8] = lg(r.opp1[p]), f[9] = lg(r.opp4[p]), f[10] = lg(r.opp24[p]), f[11] = lg(r.sold_today[p]);
    if (level)
        f[12] = float(r.inv_m4[p] / 50.0), f[13] = float(r.inv_m24[p] / 50.0), f[14] = float(r.price_m4[p] / BASE[p]),
        f[15] = float(r.price_m24[p] / BASE[p]);
    x[global + per + p] = 1;
}

// From the live observation and a History advanced to it (as at act time); sold_today = our units sold of each product since hour 0.
inline Raw raw_from(const agent::AgentObservation& o, const dc10::History& h, const int* sold_today) {
    Raw r;
    r.day = o.day, r.hour = o.hour, r.units = o.self().n_units, r.quadrants = o.self().n_quadrants;
    r.money = o.self().money, r.opp_money = o.opponent().money;
    const auto& f = o.self();
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = f.tiles[y][x];
            if (t.has_animal) {
                const AnimalDef& a = ANIMALS[t.what - GOOSE];
                r.field[a.product] += t.yield_units;
                const int since = o.day + 1 - t.planted_day - a.first_yield_day;
                if (since >= 0 && since % a.interval == 0) ++r.due[a.product];
            } else if (t.kind == T_PLANT && t.what < N_CROPS) {
                const CropDef& c = CROPS[t.what];
                if (o.day - t.planted_day >= c.first_yield_day) r.field[t.what] += t.yield_units;
                if (c.ongoing) {
                    const int since = o.day + 1 - t.planted_day - c.first_yield_day;
                    if (since >= 0 && since % c.interval == 0 && since / c.interval + 1 <= c.max_yield) ++r.due[t.what];
                }
            }
        }
    for (int p = 0; p < P; ++p) {
        r.shed[p] = o.own.shed[p];
        for (int u = 0; u < f.n_units; ++u) r.pocket[p] += o.own.inv[u][p];
        r.inv[p] = o.market.inventory[p], r.price[p] = o.market.prices[p];
        for (int s = 0; s < o.n_shops; ++s) r.shops[p] += (SHOP_MASK[o.shops[s]] >> p) & 1;
        r.opp_stock[p] = h.opponent_stock()[p];
        for (int j = 1; j <= 24 && o.step - j >= 0; ++j) {
            const int sold = std::max(0, h.flow_at(o.step - j, p));
            if (j <= 1) r.opp1[p] += sold;
            if (j <= 4) r.opp4[p] += sold;
            r.opp24[p] += sold;
        }
        r.sold_today[p] = sold_today ? sold_today[p] : 0;
        r.shed_total += r.shed[p], r.pocket_total += r.pocket[p];
    }
    return r;
}

// Deterministic uniform in [0, 1) per (seed, step, product).
inline double uniform(uint64_t seed, int step, int p) {
    uint64_t z = seed + 0x9E3779B97F4A7C15ull * uint64_t(step * P + p + 1);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull, z = (z ^ (z >> 27)) * 0x94D049BB133111EBull, z ^= z >> 31;
    return double(z >> 11) * (1.0 / 9007199254740992.0);
}

// Units of each product M&M would sell this hour: sampled, or with threshold = true (needs the file's thresholds) the threshold decode.
// Level models need the Tracker, updated with this hour's observation.
// Not capped here: units deposited this hour are sellable this hour (worker
// actions run before market orders), so the caller caps by the shed stock at order time.
inline void sell_units(const Model& m, const agent::AgentObservation& o, const dc10::History& h, const int* sold_today, uint64_t seed,
                       int* units, bool threshold = false, const Tracker* tracker = nullptr) {
    Raw r = raw_from(o, h, sold_today);
    if (has_level(m.in)) {
        if (!tracker || tracker->last_step != o.step) throw std::logic_error("sellmodel: level model needs a Tracker updated this hour");
        tracker->levels(r);
    }
    std::vector<double> pr(m.k + 1);
    float x[IN_LEVEL_CAP];
    for (int p = 0; p < P; ++p) {
        features(r, p, x, m.in);
        m.probs(x, pr.data());
        if (threshold) {
            if (1 - pr[0] <= m.thresholds.at(p)) {
                units[p] = 0;
                continue;
            }
            double lot = 0;  // expected lot given a sale, rounded (units x0.94-1.12 of M&M on held-out; the median gave 0.83-1.05)
            for (int c = 1; c <= m.k; ++c) lot += c * pr[c];
            units[p] = std::max(1, int(std::lround(lot / (1 - pr[0]))));
            continue;
        }
        double u = uniform(seed, o.step, p);
        int c = 0;
        for (; c < m.k; ++c)
            if ((u -= pr[c]) < 0) break;
        units[p] = c;
    }
}

}  // namespace bcsell
