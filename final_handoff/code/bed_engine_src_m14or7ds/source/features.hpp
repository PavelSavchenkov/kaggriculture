#pragma once
// BC actor inputs: dawn observation, the DayIntent schema, and causal opponent-flow
// history only. Shared by the dataset extractor and the agent, so training and
// inference see identical features.
#include "source/history.hpp"
#include "source/intent.hpp"
#include <cmath>

namespace bcopus {
using namespace dc10;

constexpr int GLOBAL = 256;  // padded global feature width
constexpr int CROP = 48;     // crop group feature width
constexpr int ANIMAL = 32;   // animal group feature width
constexpr int FEATURES_VERSION = 4;  // 4: site capacity at global slots SITES..SITES+6
constexpr int SITES = 208;

// Nights from today until the next production of an ongoing crop or animal (99: none).
inline int days_to_production(int age, int first, int interval, int day) {
    for (int k = 0; day + k <= LAST_DAY - 1; ++k) {
        const int since = age + k + 1 - first;
        if (since >= 0 && since % interval == 0) return k;
    }
    return 99;
}

inline double mean_shed_distance(const uint8_t* cells, int size) {
    double d = 0;
    for (int m = 0; m < size; ++m) d += shed_dist(cells[m]);
    return size ? d / size : 0;
}

inline float squash(double money) { return float(std::log1p(std::max(0.0, money)) / 12.0); }

struct FarmSummary {
    int kinds[8]{};              // tile kinds
    int empty_coops = 0, empty_pastures = 0;
    int ready[N_PRODUCTS][4]{};  // producers with product ready within 0-3 days
    double distance = 0;         // mean shed distance of producers
    int plants[N_CROPS]{};
    int held_crops[N_CROPS]{};   // harvestable held product on crops
    int animals[N_ANIMALS]{};
    int held_animals[N_ANIMALS]{};
    int fertilizer_ready = 0;
};

inline FarmSummary summarize(const agent::PublicFarm& f, int day) {
    FarmSummary s;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = f.tiles[y][x];
            ++s.kinds[std::min<int>(t.kind, 7)];
            s.empty_coops += t.kind == T_COOP && !t.has_animal;
            s.empty_pastures += t.kind == T_PASTURE && !t.has_animal;
            const int age = day - t.planted_day;
            if (t.kind == T_PLANT) {
                const CropDef& c = CROPS[t.what];
                ++s.plants[t.what];
                if (age >= c.first_yield_day) s.held_crops[t.what] += t.yield_units;
                const int wait = c.ongoing ? (t.yield_units > 0 ? 0 : days_to_production(age, c.first_yield_day, c.interval, day) + 1)
                                           : std::max(0, c.first_yield_day - age);
                for (int k = wait; k < 4; ++k) ++s.ready[t.what][k];
                s.distance += shed_dist(cell_of(x, y));
            }
            if (t.has_animal) {
                const AnimalDef& a = ANIMALS[t.what - GOOSE];
                ++s.animals[t.what - GOOSE];
                s.held_animals[t.what - GOOSE] += t.yield_units;
                s.fertilizer_ready += t.fertilizer_available;
                const int wait = t.yield_units > 0 ? 0 : days_to_production(age, a.first_yield_day, a.interval, day) + 1;
                for (int k = wait; k < 4; ++k) ++s.ready[a.product][k];
                s.distance += shed_dist(cell_of(x, y));
            }
        }
    return s;
}

// Global features; `rival` is the expected opponent net sales by hour (History).
inline void global_features(const agent::AgentObservation& o, const Schema& s, const double rival[HOURS][N_PRODUCTS],
                            float out[GLOBAL], int version = FEATURES_VERSION) {
    std::fill_n(out, GLOBAL, 0.0f);
    int k = 0;
    auto put = [&](double v) { out[k++] = float(v); };
    put(o.day / 29.0);
    put(o.day == LAST_DAY);
    put((LAST_DAY - o.day) / 29.0);
    put(squash(o.self().money));
    put(squash(o.opponent().money));
    put((o.self().money - o.opponent().money) / 50000.0);
    put(o.self().n_quadrants / 4.0);
    put(o.opponent().n_quadrants / 4.0);
    put(o.self().n_quadrants < 4 ? LAND_PRICES[std::min(2, o.self().n_quadrants - 1)] / 4000.0 : 0);
    for (int i = 0; i < N_ITEMS; ++i) put(o.own.shed[i] / 50.0);
    put(o.own.shed_total / 100.0);
    for (int c = 0; c < N_CROPS; ++c) put(o.own.seeds[c] / 20.0);
    int shop_counts[N_SHOPS]{};
    for (int i = 0; i < o.n_shops; ++i) ++shop_counts[o.shops[i]];
    for (int i = 0; i < N_SHOPS; ++i) put(shop_counts[i] / 2.0);
    put(o.n_shops / 8.0);
    const FarmSummary mine = summarize(o.self(), o.day), theirs = summarize(o.opponent(), o.day);
    for (const FarmSummary* f : {&mine, &theirs}) {
        for (int i = 0; i < 8; ++i) put(f->kinds[i] / 25.0);
        for (int c = 0; c < N_CROPS; ++c) put(f->plants[c] / 25.0);
        for (int c = 0; c < N_CROPS; ++c) put(f->held_crops[c] / 50.0);
        for (int a = 0; a < N_ANIMALS; ++a) put(f->animals[a] / 10.0);
        for (int a = 0; a < N_ANIMALS; ++a) put(f->held_animals[a] / 30.0);
        put(f->fertilizer_ready / 10.0);
    }
    // Per product: price, market inventory, and the opponent's expected daily net sales.
    for (int p = 0; p < N_PRODUCTS; ++p) {
        double flow = 0;
        for (int h = 0; h < HOURS; ++h) flow += rival[h][p];
        put(o.market.prices[p] / 200.0);
        put((o.market.inventory[p] - 10000) / 500.0);
        put(flow / 50.0);
    }
    for (int a = 0; a < N_ANIMALS; ++a) put(s.unplaced_dawn[a] / 5.0);
    put(s.n_crops / 30.0);
    put(s.n_animals / 30.0);
    // v2: capacity, workload calendars, demand and affordability.
    for (const FarmSummary* f : {&mine, &theirs}) {
        put(f->empty_coops / 5.0);
        put(f->empty_pastures / 5.0);
        const int producers = f->kinds[T_PLANT] + f->animals[0] + f->animals[1] + f->animals[2];
        put(producers ? f->distance / producers / 10.0 : 0);
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int k = 0; k < 4; ++k) put(f->ready[p][k] / 20.0);
    }
    for (int p = 0; p < N_PRODUCTS; ++p) {
        double demand = p == FERTILIZER ? 0 : 1.0 / 24;
        for (int i = 0; i < o.n_shops; ++i)
            if (SHOP_MASK[o.shops[i]] & (1u << p)) demand += SHOP_MULT[o.shops[i]] / 4.0;
        put(demand);
    }
    int hires = 0;
    for (double money = o.self().money; hires < 13 && money >= fib(hires); money -= fib(hires++)) {}
    put(hires / 13.0);
    put(o.self().n_quadrants < 4 ? o.self().money / LAND_PRICES[std::min(2, o.self().n_quadrants - 1)] / 10.0 : 0);
    put(o.self().money / 5000.0);
    // v4: site capacity for today's new crops and animals (designs/day_intent.md: they
    // must fit free tiles). Older models were trained with zeros here.
    if (version < 4) return;
    if (k > SITES) std::abort();
    k = SITES;
    int one_shot = 0, harvestable = 0, weeds = 0, ongoing = 0;
    for (int i = 0; i < s.n_crops; ++i) {
        const CropGroup& g = s.crops[i];
        if (g.fresh) continue;
        if (CROPS[g.crop].ongoing) {
            ongoing += g.size;
            continue;
        }
        one_shot += g.size;
        if (turns_weed_today(g)) weeds += g.size;
        else if (g.age >= CROPS[g.crop].first_yield_day) harvestable += g.size;
    }
    put(s.open_sites / 25.0);
    put(s.land_sites / 25.0);
    put(one_shot / 25.0);     // any one-shot crop can be cleared
    put(harvestable / 25.0);
    put(weeds / 25.0);        // free today in any case
    put(ongoing / 25.0);      // clearable
    put((s.open_sites + s.land_sites + one_shot + ongoing) / 50.0);  // most sites possible today
}

// Per-tile channels of one farm (own or opponent, public tiles only).
constexpr int GRID_CHANNELS = 24;
inline void grid_features(const agent::PublicFarm& f, int day, float out[GRID_CHANNELS * BOARD * BOARD]) {
    std::fill_n(out, GRID_CHANNELS * BOARD * BOARD, 0.0f);
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = f.tiles[y][x];
            const int cell = cell_of(x, y);
            auto put = [&](int channel, double v) { out[channel * BOARD * BOARD + cell] = float(v); };
            put(std::min<int>(t.kind, 5), 1);
            const int age = day - t.planted_day;
            if (t.kind == T_PLANT) {
                const CropDef& c = CROPS[t.what];
                put(6 + t.what, 1);
                put(14, age / 20.0);
                put(15, t.yield_units / 10.0);
                put(16, t.consecutive_dry);
                put(17, fert_days_of(t, day) / 3.0);
                put(18, c.ongoing);
                put(19, age >= c.first_yield_day);
            }
            if (t.has_animal) {
                put(11 + t.what - GOOSE, 1);
                put(14, age / 20.0);
                put(15, t.yield_units / 10.0);
                put(16, t.consecutive_dry);
                put(20, t.fertilizer_available);
                put(21, t.pending_care_bonus / 5.0);
            }
            put(22, shed_dist(cell) / 10.0);
            put(23, shed_access(cell));
        }
}

inline void crop_features(const Schema& s, const CropGroup& g, float out[CROP], double price) {
    std::fill_n(out, CROP, 0.0f);
    const CropDef& d = CROPS[g.crop];
    int k = 0;
    auto put = [&](double v) { out[k++] = float(v); };
    for (int c = 0; c < N_CROPS; ++c) put(g.crop == c);
    put(d.ongoing);
    put(g.age / 20.0);
    put((g.age - d.first_yield_day) / 10.0);
    put((g.age - d.max_yield_day) / 10.0);
    put(g.yield / 10.0);
    put(d.max_yield / 10.0);
    put(g.dry);
    put(g.fert_days / 3.0);
    put(g.decaying);
    put(g.fresh);
    put(g.size / 20.0);
    put(std::log1p(g.size) / 3.0);
    for (int o = 0; o < OPTIONS; ++o) put(d.ongoing ? 0 : option_fixed(g, s.day, o));
    put(d.ongoing && can_survive_tonight(g, s.day));
    put(d.ongoing && can_die_tonight(g));
    put(d.ongoing && ongoing_fertilize_fixed(g, s.day));
    // v2: product value, deadlines and travel.
    put(price / 200.0);
    put(g.yield * price / 1000.0);
    put(d.ongoing ? 0 : (d.max_yield_day - g.age) / 5.0);  // days until a one-shot crop decays
    put(d.ongoing ? std::min(99, days_to_production(g.age, d.first_yield_day, d.interval, s.day)) / 10.0 : 0);
    put(mean_shed_distance(g.cells.data(), g.size) / 10.0);
    put(g.age >= d.first_yield_day);
    put((LAST_DAY - s.day) / 29.0);
}

inline void animal_features(const Schema& s, const AnimalGroup& g, float out[ANIMAL], double price) {
    std::fill_n(out, ANIMAL, 0.0f);
    int k = 0;
    auto put = [&](double v) { out[k++] = float(v); };
    for (int a = 0; a < N_ANIMALS; ++a) put(g.species == a);
    put(g.age / 20.0);
    put(g.held / 10.0);
    put(g.unfed);
    put(g.bonus / 5.0);
    put(g.fresh);
    put(g.size / 10.0);
    put(std::log1p(g.size) / 3.0);
    put(care_fixed(g, s.day));
    put(g.held == 0);
    // v2: product value, production timing, capacity and travel.
    const AnimalDef& a = ANIMALS[g.species];
    put(price / 200.0);
    put(g.held * price / 1000.0);
    put(g.held >= a.max_held);
    put(std::min(99, days_to_production(g.age, a.first_yield_day, a.interval, s.day)) / 10.0);
    put(mean_shed_distance(g.cells.data(), g.size) / 10.0);
    put((LAST_DAY - s.day) / 29.0);
}
}
