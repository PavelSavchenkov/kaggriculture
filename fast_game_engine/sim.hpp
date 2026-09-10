// Kaggriculture simulator — a faithful C++ port of kaggriculture.py.
//
// Design goals, in order: (1) bit-identical results to the Python interpreter,
// (2) zero heap allocation per step, (3) trivially copyable state so search can
// snapshot and fork episodes.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <algorithm>
#include <array>
#include <bit>
#if defined(KAG_EXPLICIT_AVX2) || defined(KAG_PROFILE)
#include <immintrin.h>
#endif
#include "pyrandom.hpp"

namespace kag {

inline constexpr char OFFICIAL_VERSION[] = "1.32.7";
inline constexpr char OFFICIAL_SOURCE_SHA256[] =
    "bc8a54879ef02c7ea64b8b333d6a976f0ea65c4949149d01f463f23bccee653e";

// ---------------------------------------------------------------- item space
// PRODUCTS order matches kaggriculture.py exactly. The first five products are
// also the five crops, in CROPS order, so a crop id doubles as a product id.
enum Item : uint8_t {
    WHEAT = 0, CARROT, TOMATO, STRAWBERRY, MELON, EGG, MILK, WOOL, FERTILIZER,
    GOOSE, COW, SHEEP, N_ITEMS
};
constexpr int N_PRODUCTS = 9;
constexpr int N_CROPS = 5;
constexpr int N_ANIMALS = 3;
inline bool is_crop(uint8_t i) { return i < N_CROPS; }
inline bool is_product(uint8_t i) { return i < N_PRODUCTS; }
inline bool is_animal(uint8_t i) { return i >= GOOSE && i < N_ITEMS; }

// ---------------------------------------------------------------- unit ops
enum Op : uint8_t {
    OP_PASS = 0, OP_NORTH, OP_SOUTH, OP_EAST, OP_WEST,
    OP_PICKUP, OP_DROP, OP_PLACE,
    OP_PLANT, OP_WATER, OP_HARVEST, OP_FERTILIZE, OP_DIG,
    OP_BUILD_COOP, OP_BUILD_PASTURE,
    OP_FEED, OP_COLLECT_FERTILIZER, OP_CARE, OP_INVALID
};

enum MOp : uint8_t {
    M_NONE = 0, M_HIRE, M_BUY_LAND, M_BUY_SEED, M_BUY_PRODUCT, M_BUY_ANIMAL, M_SELL
};

// ---------------------------------------------------------------- static data
struct CropDef { int seed, first_yield_day, max_yield_day, interval, max_yield; bool ongoing; };
inline constexpr CropDef CROPS[N_CROPS] = {
    {  10, 2,  4, 0, 6, false },  // WHEAT
    {  20, 2,  3, 0, 4, false },  // CARROT
    {  50, 8,  8, 1, 4, true  },  // TOMATO
    { 100, 10, 10, 2, 4, true },  // STRAWBERRY
    {  80, 10, 12, 0, 6, false }, // MELON
};

enum Structure : uint8_t { ST_COOP = 0, ST_PASTURE = 1 };
struct AnimalDef { int cost; Structure structure; int first_yield_day, interval, max_held; Item product; };
inline constexpr AnimalDef ANIMALS[N_ANIMALS] = {
    { 300, ST_COOP,    4, 1, 4, EGG  },  // GOOSE
    { 400, ST_PASTURE, 8, 2, 6, MILK },  // COW
    { 500, ST_PASTURE, 6, 3, 6, WOOL },  // SHEEP
};

enum Shape : uint8_t { F_LINEAR, F_SQ, F_SQRT, F_LOG, F_LOG10, F_HINGE };
struct MarketDef { double base; int I0; double T; Shape below_f; double below_t; Shape above_f; double above_t; };
inline constexpr MarketDef MARKET[N_PRODUCTS] = {
    {  25, 10000, 400, F_SQRT,   0.80, F_LOG,    0.20 },  // WHEAT
    {  35, 10000, 450, F_HINGE,  1.00, F_SQRT,   0.70 },  // CARROT
    {  60, 10000, 200, F_HINGE,  0.40, F_SQRT,   0.60 },  // TOMATO
    { 120, 10000, 100, F_SQRT,   0.70, F_LINEAR, 1.60 },  // STRAWBERRY
    { 250, 10000, 300, F_LOG,    0.20, F_SQ,     3.60 },  // MELON
    {  50, 10000, 332, F_HINGE,  0.40, F_LOG,    0.20 },  // EGG
    { 160, 10000, 122, F_SQRT,   0.60, F_LINEAR, 1.60 },  // MILK
    { 200, 10000, 105, F_LOG,    0.20, F_SQ,     3.20 },  // WOOL
    { 100, 10000, 200, F_LINEAR, 0.40, F_LINEAR, 0.40 },  // FERTILIZER
};

inline double shape(Shape f, double x, double t) {
    if (x < 0.0) x = 0.0;
    switch (f) {
        case F_LINEAR: return x;
        case F_SQ:     return x * x;
        case F_SQRT:   return std::sqrt(x);
        case F_LOG:    return std::log(1.0 + x);
        case F_LOG10:  return std::log10(1.0 + x);
        case F_HINGE: {
            if (t <= 0.0) return x;
            const double u = x / t;
            const double excess = std::max(0.0, u - 1.0);
            return u + 8.0 * excess * excess;
        }
    }
    return x;
}

// Amplitudes are derived constants; precompute once.
struct MarketAmp { double below, above; };
inline const std::array<MarketAmp, N_PRODUCTS>& market_amps() {
    static const std::array<MarketAmp, N_PRODUCTS> a = [] {
        std::array<MarketAmp, N_PRODUCTS> r{};
        for (int i = 0; i < N_PRODUCTS; ++i) {
            r[i].below = MARKET[i].below_t * MARKET[i].base / shape(MARKET[i].below_f, MARKET[i].T, MARKET[i].T);
            r[i].above = MARKET[i].above_t * MARKET[i].base / shape(MARKET[i].above_f, MARKET[i].T, MARKET[i].T);
        }
        return r;
    }();
    return a;
}

// Python: max(PRICE_FLOOR, int(round(price))). Python's round() on a float is
// round-half-to-even, which is exactly nearbyint's default mode.
inline int market_price_uncached(int item, int inv) {
    const MarketDef& p = MARKET[item];
    const MarketAmp& a = market_amps()[item];
    double price;
    if (inv < p.I0) price = p.base + a.below * shape(p.below_f, static_cast<double>(p.I0 - inv), p.T);
    else            price = p.base - a.above * shape(p.above_f, static_cast<double>(inv - p.I0), p.T);
    int v = static_cast<int>(std::nearbyint(price));
    return v < 1 ? 1 : v;
}

constexpr int PRICE_CACHE_MIN = 4096;
constexpr int PRICE_CACHE_MAX = 16384;
constexpr int PRICE_CACHE_SIZE = PRICE_CACHE_MAX - PRICE_CACHE_MIN + 1;

#ifndef KAG_DISABLE_PRICE_TABLE
struct PriceCache {
    std::array<std::array<int32_t, PRICE_CACHE_SIZE>, N_PRODUCTS> values{};
    PriceCache() {
        for (int product = 0; product < N_PRODUCTS; ++product)
            for (int inventory = PRICE_CACHE_MIN; inventory <= PRICE_CACHE_MAX; ++inventory)
                values[product][inventory - PRICE_CACHE_MIN] = market_price_uncached(product, inventory);
    }
};
inline const PriceCache PRICE_CACHE;
#endif

inline int market_price(int item, int inv) {
#ifdef KAG_DISABLE_PRICE_TABLE
    return market_price_uncached(item, inv);
#else
    if (static_cast<unsigned>(inv - PRICE_CACHE_MIN) < PRICE_CACHE_SIZE)
        return PRICE_CACHE.values[item][inv - PRICE_CACHE_MIN];
    return market_price_uncached(item, inv);
#endif
}

// ---------------------------------------------------------------- shops / town
enum ShopId : uint8_t {
    SHOP_BAKERY = 0, SHOP_BRUNCH_SPOT, SHOP_FARMERS_MARKET, SHOP_ICE_CREAM_SHOP,
    SHOP_PET_CAFE, SHOP_PIZZA_SHOP, SHOP_SMOOTHIE_SHOP, SHOP_YARN_STORE, N_SHOPS
};
// NOTE: this array is in sorted(SHOPS) order, because the environment unlocks
// with `rng.choice(sorted(SHOPS))`.
inline constexpr uint16_t SHOP_MASK[N_SHOPS] = {
    (1u << EGG) | (1u << WHEAT),                                        // BAKERY
    (1u << EGG) | (1u << WHEAT) | (1u << STRAWBERRY),                   // BRUNCH_SPOT
    (1u << WHEAT) | (1u << CARROT) | (1u << TOMATO) | (1u << STRAWBERRY),// FARMERS_MARKET
    (1u << STRAWBERRY) | (1u << MILK) | (1u << WHEAT),                  // ICE_CREAM_SHOP
    (1u << CARROT),                                                     // PET_CAFE
    (1u << MILK) | (1u << TOMATO) | (1u << WHEAT),                      // PIZZA_SHOP
    (1u << STRAWBERRY) | (1u << MILK),                                  // SMOOTHIE_SHOP
    (1u << WOOL),                                                       // YARN_STORE
};
inline constexpr int SHOP_MULT[N_SHOPS] = { 1, 1, 1, 1, 2, 1, 1, 2 };  // single-product shops pull 2x
constexpr int MAX_SHOP_INSTANCES = 8;

// ---------------------------------------------------------------- config
struct Config {
    int episode_steps = 720;
    int board_size = 10;
    int starting_money = 3000;
    int max_orders = 10;
    int turns_per_day = 24;
    int shed_capacity = 100;
    double weed_chance = 0.005;
    int shop_unlock_interval = 3;
    int shop_sell_interval = 4;
    int center_sell_interval = 24;
    int hire_mult = 1;
    uint64_t seed = 0;
};

// ---------------------------------------------------------------- state
enum TileKind : uint8_t { T_EMPTY = 0, T_LOCKED, T_WEED, T_COOP, T_PASTURE, T_PLANT };

struct Tile {
    TileKind kind = T_EMPTY;
    uint8_t  what = 0;          // crop id (PLANT) or animal id (COOP/PASTURE with animal)
    bool     has_animal = false;
    bool     watered_today = false;
    bool     fed_today = false;
    bool     cared_today = false;
    bool     fertilizer_available = false;
    int8_t   consecutive_dry = 0;    // unwatered (plant) / unfed (animal)
    int8_t   yield_units = 0;
    int8_t   pending_care_bonus = 0;
    int16_t  planted_day = 0;        // or placed_day
    int32_t  max_lifespan_step = -1;
    int16_t  fertilized_until_day = -1;
};

constexpr int MAX_UNITS = 40;   // farmer + hands; hires are Fibonacci-priced so this is ample
constexpr int BOARD = 10;
#ifdef KAG_WIDE_COUNTS
using Count = int32_t;
#else
using Count = int16_t;
#endif

struct Farm {
    double money = 0;
    Tile tiles[BOARD][BOARD];
    int8_t pos_x[MAX_UNITS], pos_y[MAX_UNITS];
    int n_units = 1;                 // index 0 is the main farmer
    int n_quadrants = 1;             // NW always unlocked
    int hires_today = 0;
    uint64_t decay_mask[2] = {0, 0};
    uint64_t plant_mask[2] = {0, 0};
    uint64_t animal_mask[2] = {0, 0};
    uint64_t empty_mask[2] = {0, 0};
    int next_decay_step = std::numeric_limits<int>::max();

    Count shed[N_ITEMS] = {0};
    int shed_total = 0;
    Count seeds[N_CROPS] = {0};
    Count inv[MAX_UNITS][N_ITEMS] = {{0}};
    // Per-unit inventories are Python dicts, and several code paths iterate
    // them in INSERTION order. That order decides which items win the last
    // slots when the 100-item shed cap binds, so it is load-bearing, not
    // cosmetic, and must be modelled explicitly.
    uint8_t inv_keys[MAX_UNITS][N_ITEMS] = {{0}};
    uint8_t inv_nkeys[MAX_UNITS] = {0};

    // Instrumentation only - never read by the interpreter, so behaviour and
    // parity are unaffected. `discarded` is the production the 100-item shed
    // cap silently destroyed, which is otherwise invisible to search.
    int32_t discarded[N_ITEMS] = {0};
    int32_t produced[N_ITEMS] = {0};
    int32_t sold_units[N_ITEMS] = {0};
    double  sell_revenue = 0;   // coins actually received from SELLs
    double  total_spend = 0;    // coins actually paid out

    void inv_add(int u, int item, int n) {
        if (n <= 0) return;
        if (inv[u][item] == 0) inv_keys[u][inv_nkeys[u]++] = (uint8_t)item;
        inv[u][item] += static_cast<Count>(n);
    }
    void inv_erase(int u, int item) {
        inv[u][item] = 0;
        int k = 0;
        while (k < inv_nkeys[u] && inv_keys[u][k] != item) ++k;
        if (k == inv_nkeys[u]) return;
        for (int j = k; j + 1 < inv_nkeys[u]; ++j) inv_keys[u][j] = inv_keys[u][j + 1];
        inv_nkeys[u]--;
    }
    bool inv_take(int u, int item, int n) {
        if (inv[u][item] < n) return false;
        inv[u][item] -= static_cast<Count>(n);
        if (inv[u][item] == 0) inv_erase(u, item);
        return true;
    }
    void inv_clear(int u) { for (int i = 0; i < N_ITEMS; ++i) inv[u][i] = 0; inv_nkeys[u] = 0; }
};

struct Market {
    int32_t inventory[N_PRODUCTS];
    int32_t prices[N_PRODUCTS];
};

struct State {
    Farm farms[2];
    Market market;
    uint8_t shops[MAX_SHOP_INSTANCES];
    int n_shops = 0;
    int step = 0;
    int day = 0;
    int hour = 0;
    bool done = false;
};
static_assert(std::is_trivially_copyable_v<State>);

// ---------------------------------------------------------------- actions
struct UnitAction { uint8_t op = OP_PASS; uint8_t arg = 0; int32_t n = 1; };
struct Order { uint8_t op = M_NONE; uint8_t item = 0; int32_t n = 0; };

struct Action {
    UnitAction units[MAX_UNITS];     // units[0] = farmer
    int n_units = 1;
    Order orders[16];
    int n_orders = 0;
    uint8_t plant_demand[N_CROPS] = {0};
    uint8_t plant_mask = 0;
    bool metadata_ready = false;
    void clear() {
        n_units = 1;
        units[0] = UnitAction{};
        n_orders = 0;
        plant_mask = 0;
        metadata_ready = false;
    }
    void finalize() {
        std::fill_n(plant_demand, N_CROPS, uint8_t{0});
        plant_mask = 0;
        for (int i = 0; i < n_units; ++i)
            if (units[i].op == OP_PLANT && units[i].arg < N_CROPS) {
                ++plant_demand[units[i].arg];
                plant_mask = static_cast<uint8_t>(
                    plant_mask | static_cast<uint8_t>(uint8_t{1} <<
                                                       units[i].arg));
            }
        metadata_ready = true;
    }
};
static_assert(std::is_trivially_copyable_v<Action>);

// ---------------------------------------------------------------- helpers
inline int quadrant_of(int x, int y, int bs) {
    // 0=NW 1=NE 2=SW 3=SE
    int half = bs / 2;
    return (y < half ? 0 : 2) + (x < half ? 0 : 1);
}
// Land is unlocked in the order NE, SW, SE.
inline constexpr int LAND_ORDER[3] = { 1, 2, 3 };
inline constexpr int LAND_PRICES[3] = { 1000, 2000, 4000 };

inline void shed_access_tiles(int bs, int out[4][2]) {
    int h = bs / 2;
    out[0][0] = h - 1; out[0][1] = h - 1;
    out[1][0] = h;     out[1][1] = h - 1;
    out[2][0] = h - 1; out[2][1] = h;
    out[3][0] = h;     out[3][1] = h;
}
inline bool is_shed_adjacent(int x, int y, int bs) {
    int h = bs / 2;
    return (x == h - 1 || x == h) && (y == h - 1 || y == h);
}

inline int fib(int n) { int a = 1, b = 1; for (int i = 0; i < n; ++i) { int t = b; b = a + b; a = t; } return a; }

class Sim {
public:
#ifdef KAG_PROFILE
    struct PhaseProfile {
        uint64_t units = 0;
        uint64_t market = 0;
        uint64_t town = 0;
        uint64_t decay = 0;
        uint64_t end_day = 0;
    } profile;
#endif
    Config cfg;
    State st;

    explicit Sim(const Config& c = Config{}) : cfg(c) { reset(); }

    void reset() {
        if (cfg.board_size != BOARD || cfg.max_orders < 1 || cfg.max_orders > 16 ||
            cfg.turns_per_day < 1 || cfg.episode_steps < 1 ||
            cfg.shop_unlock_interval < 1 || cfg.shop_sell_interval < 1 ||
            cfg.center_sell_interval < 1)
            std::abort();
        st = State{};
        dirty_products_ = 0;
        next_shop_sell_step_ = 0;
        next_center_sell_step_ = 0;
        next_shop_unlock_day_ = cfg.shop_unlock_interval;
        int h = cfg.board_size / 2;
        for (int p = 0; p < 2; ++p) {
            Farm& f = st.farms[p];
            f.money = cfg.starting_money;
            for (int y = 0; y < cfg.board_size; ++y)
                for (int x = 0; x < cfg.board_size; ++x)
                    if (quadrant_of(x, y, cfg.board_size) == 0) {
                        f.tiles[y][x].kind = T_EMPTY;
                        set_mask(f.empty_mask, x, y, true);
                    } else {
                        f.tiles[y][x].kind = T_LOCKED;
                    }
            f.n_units = 1;
            f.pos_x[0] = static_cast<int8_t>(h - 1);
            f.pos_y[0] = static_cast<int8_t>(h - 1);
        }
        for (int i = 0; i < N_PRODUCTS; ++i) {
            st.market.inventory[i] = MARKET[i].I0;
            st.market.prices[i] = static_cast<int>(MARKET[i].base);
        }
    }

    double reward(int p) const { return st.farms[p].money; }

    uint64_t parity_hash() const {
        struct Hash {
            uint64_t value = 14695981039346656037ull;
            void add(int64_t input) {
                uint64_t word = static_cast<uint64_t>(input);
                for (int shift = 0; shift < 64; shift += 8) {
                    value ^= (word >> shift) & 0xffu;
                    value *= 1099511628211ull;
                }
            }
        } hash;

        hash.add(st.step);
        hash.add(st.day);
        hash.add(st.hour);
        hash.add(st.done);
        hash.add(st.n_shops);
        for (int i = 0; i < st.n_shops; ++i) hash.add(st.shops[i]);
        for (int i = 0; i < N_PRODUCTS; ++i) hash.add(st.market.inventory[i]);
        for (int i = 0; i < N_PRODUCTS; ++i) hash.add(st.market.prices[i]);

        for (const Farm& farm : st.farms) {
            hash.add(static_cast<int64_t>(farm.money));
            hash.add(farm.n_units);
            hash.add(farm.n_quadrants);
            hash.add(farm.hires_today);
            hash.add(0);
            for (int quadrant = 1; quadrant < farm.n_quadrants; ++quadrant) hash.add(quadrant);
            for (int unit = 0; unit < farm.n_units; ++unit) {
                hash.add(farm.pos_x[unit]);
                hash.add(farm.pos_y[unit]);
            }
            hash.add(farm.shed_total);
            for (int item = 0; item < N_ITEMS; ++item) hash.add(farm.shed[item]);
            for (int crop = 0; crop < N_CROPS; ++crop) hash.add(farm.seeds[crop]);
            hash.add(farm.n_units);
            for (int unit = 0; unit < farm.n_units; ++unit) {
                hash.add(farm.inv_nkeys[unit]);
                for (int key = 0; key < farm.inv_nkeys[unit]; ++key) {
                    int item = farm.inv_keys[unit][key];
                    hash.add(item);
                    hash.add(farm.inv[unit][item]);
                }
            }
            for (int y = 0; y < cfg.board_size; ++y) {
                for (int x = 0; x < cfg.board_size; ++x) {
                    const Tile& tile = farm.tiles[y][x];
                    hash.add(tile.kind);
                    if (tile.kind == T_EMPTY || tile.kind == T_LOCKED) {
                        for (int i = 0; i < 12; ++i) hash.add(0);
                        continue;
                    }
                    hash.add(tile.kind == T_PLANT || tile.has_animal ? tile.what : 0);
                    hash.add(tile.has_animal);
                    hash.add(tile.watered_today);
                    hash.add(tile.fed_today);
                    hash.add(tile.cared_today);
                    hash.add(tile.fertilizer_available);
                    hash.add(tile.consecutive_dry);
                    hash.add(tile.yield_units);
                    hash.add(tile.pending_care_bonus);
                    hash.add(tile.planted_day);
                    hash.add(tile.max_lifespan_step);
                    hash.add(tile.fertilized_until_day);
                }
            }
        }
        return hash.value;
    }

    // One environment step, given both players' actions.
    void step(const Action& a0, const Action& a1) {
        if (st.done) return;
        const Action* acts[2] = { &a0, &a1 };
        int step_i = st.step;
        int day = st.day;

#ifdef KAG_PROFILE
        auto ticks = [] { _mm_lfence(); return __rdtsc(); };
        uint64_t started = ticks();
#endif
        for (int p = 0; p < 2; ++p) apply_unit_actions(p, *acts[p], day);
#ifdef KAG_PROFILE
        profile.units += ticks() - started;
        started = ticks();
#endif
        if (a0.n_orders != 0 || a1.n_orders != 0) process_market(*acts[0], *acts[1]);
#ifdef KAG_PROFILE
        profile.market += ticks() - started;
        started = ticks();
#endif
        town_consume(step_i);
#ifdef KAG_PROFILE
        profile.town += ticks() - started;
        started = ticks();
#endif
        for (int p = 0; p < 2; ++p) decay_plants(st.farms[p], step_i);
#ifdef KAG_PROFILE
        profile.decay += ticks() - started;
        started = ticks();
#endif
        const bool last_hour = st.hour + 1 == cfg.turns_per_day;
        if (last_hour) end_of_day(day);
#ifdef KAG_PROFILE
        profile.end_day += ticks() - started;
#endif

        st.step = step_i + 1;
        st.day += last_hour;
        st.hour = last_hour ? 0 : st.hour + 1;
        if (step_i >= cfg.episode_steps - 2) st.done = true;
    }

    struct SoloActionOutcome {
        int requested_unit_actions = 0;
        int successful_unit_actions = 0;
        int requested_order_units = 0;
        int successful_order_units = 0;
    };

    struct JointActionOutcome {
        SoloActionOutcome players[2];
    };

    // Read-only exact diagnostic for the unit and market portions of a joint
    // active-opponent step. Unlike two independent solo diagnostics, this
    // preserves order-position lockstep, shared quotes, and player commit
    // order. It intentionally stops before town, decay, and day-end events.
    JointActionOutcome diagnose_joint_actions(
        const Action& first, const Action& second) const {
        Sim copy = *this;
        const Action* actions[2] = {&first, &second};
        JointActionOutcome result;
        for (int player = 0; player < 2; ++player) {
            const SoloActionOutcome solo = diagnose_solo_action(
                player, *actions[player], true);
            result.players[player].requested_unit_actions =
                solo.requested_unit_actions;
            result.players[player].successful_unit_actions =
                solo.successful_unit_actions;
            copy.apply_unit_actions(player, *actions[player], copy.st.day);
        }

        struct DiagnosticOrder {
            uint8_t type = M_NONE;
            uint8_t item = 0;
            int32_t remaining = 0;
            bool live = false;
        };
        int order_counts[2];
        for (int player = 0; player < 2; ++player)
            order_counts[player] = std::min(
                actions[player]->n_orders, copy.cfg.max_orders);
        const int max_orders = std::max(order_counts[0], order_counts[1]);
        for (int index = 0; index < max_orders; ++index) {
            DiagnosticOrder orders[2];
            for (int player = 0; player < 2; ++player) {
                if (index >= order_counts[player]) continue;
                const Order& order = actions[player]->orders[index];
                if (order.op == M_HIRE || order.op == M_BUY_LAND) {
                    orders[player] = {order.op, 0, 1, true};
                    ++result.players[player].requested_order_units;
                } else if (order.op != M_NONE && order.n > 0) {
                    orders[player] = {
                        order.op, order.item, order.n, true};
                    result.players[player].requested_order_units += order.n;
                }
            }

            for (int player = 0; player < 2; ++player) {
                if (!orders[player].live) continue;
                Farm& farm = copy.st.farms[player];
                if (orders[player].type == M_HIRE) {
                    const int before = farm.n_units;
                    copy.do_hire(farm);
                    result.players[player].successful_order_units +=
                        farm.n_units != before;
                    orders[player].live = false;
                } else if (orders[player].type == M_BUY_LAND) {
                    const int before = farm.n_quadrants;
                    copy.do_buy_land(farm);
                    result.players[player].successful_order_units +=
                        farm.n_quadrants != before;
                    orders[player].live = false;
                }
            }

            for (;;) {
                struct Quote {
                    bool valid = false;
                    uint8_t type = M_NONE;
                    uint8_t item = 0;
                    int price = 0;
                } quotes[2];
                for (int player = 0; player < 2; ++player) {
                    if (!orders[player].live ||
                        orders[player].remaining <= 0)
                        continue;
                    const uint8_t type = orders[player].type;
                    const uint8_t item = orders[player].item;
                    if (type == M_SELL && is_product(item)) {
                        quotes[player] = {true, type, item, market_price(
                            item, copy.st.market.inventory[item])};
                    } else if (type == M_BUY_PRODUCT &&
                               (item == WHEAT || item == FERTILIZER)) {
                        quotes[player] = {true, type, item, market_price(
                            item, copy.st.market.inventory[item] - 1)};
                    } else if (type == M_BUY_SEED && is_crop(item)) {
                        quotes[player] = {true, type, item,
                                          CROPS[item].seed};
                    } else if (type == M_BUY_ANIMAL && is_animal(item)) {
                        quotes[player] = {true, type, item,
                            ANIMALS[item - GOOSE].cost};
                    } else {
                        orders[player].live = false;
                    }
                }
                if (!quotes[0].valid && !quotes[1].valid) break;
                bool committed = false;
                for (int player = 0; player < 2; ++player) {
                    const Quote& quote = quotes[player];
                    if (!quote.valid) continue;
                    if (copy.commit_unit(quote.type, quote.item, quote.price,
                                         copy.st.farms[player])) {
                        --orders[player].remaining;
                        ++result.players[player].successful_order_units;
                        committed = true;
                    } else {
                        orders[player].live = false;
                    }
                }
                if (!committed) break;
            }
            copy.refresh_prices();
        }
        return result;
    }

    // Return joint actions with every ineffective operation removed and every
    // market quantity clipped to the number actually committed. Unlike the
    // solo sanitizer below, this preserves active-opponent order lockstep,
    // shared quotes, player commit order, and market-line positions. Stepping
    // the returned pair from this state is parity-equivalent to stepping the
    // requested pair (town/day-end effects included by the eventual step).
    std::array<Action, 2> sanitize_joint_actions(
        const Action& first, const Action& second) const {
        Sim copy = *this;
        const Action* requested[2] = {&first, &second};
        std::array<Action, 2> result;
        for (int player = 0; player < 2; ++player) {
            result[player].clear();
            result[player].n_units = copy.st.farms[player].n_units;
            for (int unit = 0; unit < result[player].n_units; ++unit)
                result[player].units[unit] = {};

            const Farm& farm = copy.st.farms[player];
            int demand[N_CROPS] = {0};
            for (int unit = 0; unit < requested[player]->n_units; ++unit)
                if (requested[player]->units[unit].op == OP_PLANT &&
                    requested[player]->units[unit].arg < N_CROPS)
                    ++demand[requested[player]->units[unit].arg];
            bool blocked[N_CROPS];
            for (int crop = 0; crop < N_CROPS; ++crop)
                blocked[crop] = demand[crop] > farm.seeds[crop];

            const int units = std::min(requested[player]->n_units,
                                       farm.n_units);
            Farm& mutable_farm = copy.st.farms[player];
            for (int unit = 0; unit < units; ++unit) {
                const UnitAction& value = requested[player]->units[unit];
                if (value.op == OP_PASS ||
                    (value.op == OP_PLANT && value.arg < N_CROPS &&
                     blocked[value.arg]))
                    continue;
                const uint64_t before = copy.parity_hash();
                const int before_item = value.arg < N_ITEMS ?
                    mutable_farm.inv[unit][value.arg] : 0;
                copy.apply_unit(mutable_farm, unit, value, copy.st.day);
                if (copy.parity_hash() == before) continue;

                UnitAction canonical{value.op, 0, 1};
                if (value.op == OP_PLANT) canonical.arg = value.arg;
                if (value.op == OP_PICKUP) {
                    canonical.arg = value.arg;
                    canonical.n = mutable_farm.inv[unit][value.arg] -
                                  before_item;
                } else if (value.op == OP_PLACE) {
                    canonical.arg = value.arg;
                    canonical.n = is_animal(value.arg) ? 1 :
                        before_item - mutable_farm.inv[unit][value.arg];
                }
                result[player].units[unit] = canonical;
            }
            result[player].finalize();
            result[player].n_orders = std::min(requested[player]->n_orders,
                                               copy.cfg.max_orders);
            for (int index = 0; index < result[player].n_orders; ++index)
                result[player].orders[index] = {};
        }

        struct CanonicalOrder {
            uint8_t type = M_NONE;
            uint8_t item = 0;
            int32_t remaining = 0;
            bool live = false;
        };
        const int max_orders = std::max(result[0].n_orders,
                                        result[1].n_orders);
        for (int index = 0; index < max_orders; ++index) {
            CanonicalOrder orders[2];
            for (int player = 0; player < 2; ++player) {
                if (index >= result[player].n_orders) continue;
                const Order& order = requested[player]->orders[index];
                if (order.op == M_HIRE || order.op == M_BUY_LAND) {
                    orders[player] = {order.op, 0, 1, true};
                } else if (order.op != M_NONE && order.n > 0) {
                    orders[player] = {order.op, order.item, order.n, true};
                }
            }

            for (int player = 0; player < 2; ++player) {
                if (!orders[player].live) continue;
                Farm& farm = copy.st.farms[player];
                if (orders[player].type == M_HIRE) {
                    const int before = farm.n_units;
                    copy.do_hire(farm);
                    if (farm.n_units != before)
                        result[player].orders[index] = {M_HIRE, 0, 1};
                    orders[player].live = false;
                } else if (orders[player].type == M_BUY_LAND) {
                    const int before = farm.n_quadrants;
                    copy.do_buy_land(farm);
                    if (farm.n_quadrants != before)
                        result[player].orders[index] = {M_BUY_LAND, 0, 1};
                    orders[player].live = false;
                }
            }

            int accepted[2] = {0, 0};
            for (;;) {
                struct Quote {
                    bool valid = false;
                    uint8_t type = M_NONE;
                    uint8_t item = 0;
                    int price = 0;
                } quotes[2];
                for (int player = 0; player < 2; ++player) {
                    if (!orders[player].live || orders[player].remaining <= 0)
                        continue;
                    const uint8_t type = orders[player].type;
                    const uint8_t item = orders[player].item;
                    if (type == M_SELL && is_product(item)) {
                        quotes[player] = {true, type, item, market_price(
                            item, copy.st.market.inventory[item])};
                    } else if (type == M_BUY_PRODUCT &&
                               (item == WHEAT || item == FERTILIZER)) {
                        quotes[player] = {true, type, item, market_price(
                            item, copy.st.market.inventory[item] - 1)};
                    } else if (type == M_BUY_SEED && is_crop(item)) {
                        quotes[player] = {true, type, item,
                                          CROPS[item].seed};
                    } else if (type == M_BUY_ANIMAL && is_animal(item)) {
                        quotes[player] = {true, type, item,
                            ANIMALS[item - GOOSE].cost};
                    } else {
                        orders[player].live = false;
                    }
                }
                if (!quotes[0].valid && !quotes[1].valid) break;
                bool committed = false;
                for (int player = 0; player < 2; ++player) {
                    const Quote& quote = quotes[player];
                    if (!quote.valid) continue;
                    if (copy.commit_unit(quote.type, quote.item, quote.price,
                                         copy.st.farms[player])) {
                        --orders[player].remaining;
                        ++accepted[player];
                        committed = true;
                    } else {
                        orders[player].live = false;
                    }
                }
                if (!committed) break;
            }
            for (int player = 0; player < 2; ++player)
                if (accepted[player] > 0)
                    result[player].orders[index] = {
                        orders[player].type, orders[player].item,
                        accepted[player]};
            copy.refresh_prices();
        }
        return result;
    }

    // Return the largest order-preserving subset that succeeds against a PASS
    // opponent from the current state. This is a read-only policy helper; it
    // does not alter the simulator or relax active-opponent diagnostics.
    Action sanitize_solo_action(int player, const Action& requested) const {
        if (player < 0 || player > 1) std::abort();
        Sim copy = *this;
        Farm& farm = copy.st.farms[player];
        Action result;
        result.clear();
        result.n_units = farm.n_units;
        for (int unit = 0; unit < result.n_units; ++unit)
            result.units[unit] = {};

        int demand[N_CROPS] = {0};
        for (int unit = 0; unit < requested.n_units; ++unit)
            if (requested.units[unit].op == OP_PLANT &&
                requested.units[unit].arg < N_CROPS)
                ++demand[requested.units[unit].arg];
        bool blocked[N_CROPS];
        for (int crop = 0; crop < N_CROPS; ++crop)
            blocked[crop] = demand[crop] > farm.seeds[crop];

        const int units = std::min(requested.n_units, farm.n_units);
        for (int unit = 0; unit < units; ++unit) {
            const UnitAction& value = requested.units[unit];
            if (value.op == OP_PASS ||
                (value.op == OP_PLANT && value.arg < N_CROPS &&
                 blocked[value.arg]))
                continue;
            const uint64_t before = copy.parity_hash();
            copy.apply_unit(farm, unit, value, copy.st.day);
            if (copy.parity_hash() != before) result.units[unit] = value;
        }
        result.finalize();

        result.n_orders = std::min(requested.n_orders, copy.cfg.max_orders);
        for (int index = 0; index < result.n_orders; ++index) {
            result.orders[index] = {};
            const Order& order = requested.orders[index];
            if (order.op == M_HIRE) {
                const int before = farm.n_units;
                copy.do_hire(farm);
                if (farm.n_units != before) result.orders[index] = order;
                copy.refresh_prices();
                continue;
            }
            if (order.op == M_BUY_LAND) {
                const int before = farm.n_quadrants;
                copy.do_buy_land(farm);
                if (farm.n_quadrants != before) result.orders[index] = order;
                copy.refresh_prices();
                continue;
            }
            if (order.op == M_NONE || order.n <= 0) {
                copy.refresh_prices();
                continue;
            }
            int accepted = 0;
            for (int unit = 0; unit < order.n; ++unit) {
                int price = 0;
                bool valid = false;
                if (order.op == M_SELL && is_product(order.item)) {
                    price = market_price(order.item,
                        copy.st.market.inventory[order.item]);
                    valid = true;
                } else if (order.op == M_BUY_PRODUCT &&
                           (order.item == WHEAT ||
                            order.item == FERTILIZER)) {
                    price = market_price(order.item,
                        copy.st.market.inventory[order.item] - 1);
                    valid = true;
                } else if (order.op == M_BUY_SEED && is_crop(order.item)) {
                    price = CROPS[order.item].seed;
                    valid = true;
                } else if (order.op == M_BUY_ANIMAL &&
                           is_animal(order.item)) {
                    price = ANIMALS[order.item - GOOSE].cost;
                    valid = true;
                }
                if (!valid || !copy.commit_unit(
                        order.op, order.item, price, farm))
                    break;
                ++accepted;
            }
            if (accepted > 0)
                result.orders[index] = {
                    order.op, order.item, accepted};
            copy.refresh_prices();
        }
        return result;
    }

    // Read-only exact diagnostic for one player against a PASS opponent. It
    // reproduces the unit and market portions of the next step on a copy and
    // does not run town, decay, or end-of-day events. Normal simulation pays
    // no runtime cost unless this method is called.
    SoloActionOutcome diagnose_solo_action(
        int player, const Action& action,
        bool localized_unit_hash = false) const {
        if (player < 0 || player > 1) std::abort();
        Sim copy = *this;
        Farm& farm = copy.st.farms[player];
        SoloActionOutcome result;
        const int applied = std::min(action.n_units, farm.n_units);
        int demand[N_CROPS] = {0};
        if (action.metadata_ready) {
            for (int crop = 0; crop < N_CROPS; ++crop)
                demand[crop] = action.plant_demand[crop];
        } else {
            for (int unit = 0; unit < action.n_units; ++unit)
                if (action.units[unit].op == OP_PLANT &&
                    action.units[unit].arg < N_CROPS)
                    ++demand[action.units[unit].arg];
        }
        bool blocked[N_CROPS];
        for (int crop = 0; crop < N_CROPS; ++crop)
            blocked[crop] = demand[crop] > farm.seeds[crop];
        auto unit_effect_hash = [](const Farm& current, int unit) {
            struct Hash {
                uint64_t value = 14695981039346656037ull;
                void add(int64_t input) {
                    const uint64_t word = static_cast<uint64_t>(input);
                    for (int shift = 0; shift < 64; shift += 8) {
                        value ^= (word >> shift) & 0xffu;
                        value *= 1099511628211ull;
                    }
                }
            } hash;
            hash.add(current.pos_x[unit]);
            hash.add(current.pos_y[unit]);
            hash.add(current.shed_total);
            for (int item = 0; item < N_ITEMS; ++item)
                hash.add(current.shed[item]);
            for (int crop = 0; crop < N_CROPS; ++crop)
                hash.add(current.seeds[crop]);
            hash.add(current.inv_nkeys[unit]);
            for (int key = 0; key < current.inv_nkeys[unit]; ++key) {
                const int item = current.inv_keys[unit][key];
                hash.add(item);
                hash.add(current.inv[unit][item]);
            }
            const Tile& tile = current.tiles[current.pos_y[unit]]
                [current.pos_x[unit]];
            hash.add(tile.kind);
            hash.add(tile.what);
            hash.add(tile.has_animal);
            hash.add(tile.watered_today);
            hash.add(tile.fed_today);
            hash.add(tile.cared_today);
            hash.add(tile.fertilizer_available);
            hash.add(tile.consecutive_dry);
            hash.add(tile.yield_units);
            hash.add(tile.pending_care_bonus);
            hash.add(tile.planted_day);
            hash.add(tile.max_lifespan_step);
            hash.add(tile.fertilized_until_day);
            return hash.value;
        };
        for (int unit = 0; unit < applied; ++unit) {
            const UnitAction& value = action.units[unit];
            if (value.op == OP_PASS) continue;
            ++result.requested_unit_actions;
            if (value.op == OP_PLANT && value.arg < N_CROPS &&
                blocked[value.arg])
                continue;
            const uint64_t before = localized_unit_hash ?
                unit_effect_hash(farm, unit) : copy.parity_hash();
            copy.apply_unit(farm, unit, value, copy.st.day);
            const uint64_t after = localized_unit_hash ?
                unit_effect_hash(farm, unit) : copy.parity_hash();
            result.successful_unit_actions += before != after;
        }

        const int orders = std::min(action.n_orders, copy.cfg.max_orders);
        for (int index = 0; index < orders; ++index) {
            const Order& order = action.orders[index];
            if (order.op == M_HIRE) {
                ++result.requested_order_units;
                const int before = farm.n_units;
                copy.do_hire(farm);
                result.successful_order_units += farm.n_units != before;
                copy.refresh_prices();
                continue;
            }
            if (order.op == M_BUY_LAND) {
                ++result.requested_order_units;
                const int before = farm.n_quadrants;
                copy.do_buy_land(farm);
                result.successful_order_units += farm.n_quadrants != before;
                copy.refresh_prices();
                continue;
            }
            if (order.op == M_NONE || order.n <= 0) {
                copy.refresh_prices();
                continue;
            }
            result.requested_order_units += order.n;
            for (int unit = 0; unit < order.n; ++unit) {
                int price = 0;
                bool valid = false;
                if (order.op == M_SELL && is_product(order.item)) {
                    price = market_price(order.item,
                                         copy.st.market.inventory[order.item]);
                    valid = true;
                } else if (order.op == M_BUY_PRODUCT &&
                           (order.item == WHEAT || order.item == FERTILIZER)) {
                    price = market_price(
                        order.item, copy.st.market.inventory[order.item] - 1);
                    valid = true;
                } else if (order.op == M_BUY_SEED && is_crop(order.item)) {
                    price = CROPS[order.item].seed;
                    valid = true;
                } else if (order.op == M_BUY_ANIMAL &&
                           is_animal(order.item)) {
                    price = ANIMALS[order.item - GOOSE].cost;
                    valid = true;
                }
                if (!valid || !copy.commit_unit(
                        order.op, order.item, price, farm))
                    break;
                ++result.successful_order_units;
            }
            copy.refresh_prices();
        }
        return result;
    }

private:
    uint16_t dirty_products_ = 0;
    int next_shop_sell_step_ = 0;
    int next_center_sell_step_ = 0;
    int next_shop_unlock_day_ = 0;

    static int tile_index(int x, int y) { return y * BOARD + x; }

    static void set_mask(uint64_t mask[2], int x, int y, bool enabled) {
        const int index = tile_index(x, y);
        const uint64_t bit = uint64_t{1} << (index & 63);
        if (enabled) mask[index >> 6] |= bit;
        else mask[index >> 6] &= ~bit;
    }

    static void set_decay(Farm& farm, int x, int y, bool enabled) {
        set_mask(farm.decay_mask, x, y, enabled);
        if (enabled)
            farm.next_decay_step = std::min(farm.next_decay_step,
                                             farm.tiles[y][x].max_lifespan_step);
    }

    void mark_price_dirty(int item) {
        dirty_products_ = static_cast<uint16_t>(
            dirty_products_ | static_cast<uint16_t>(uint16_t{1} << item));
    }

    // ------------------------------------------------------------ unit actions
    void apply_unit_actions(int p, const Action& a, int day) {
        Farm& f = st.farms[p];
        // Atomic PLANT validation: if requests for a crop exceed seeds held,
        // every PLANT of that crop this turn is dropped.
        // Demand is counted over every submitted unit action, including ones
        // addressed to hands that do not exist (matching the Python, where the
        // blocked set is computed before the position lookup no-ops).
        int n = std::min(a.n_units, f.n_units);
        if (a.metadata_ready && a.plant_mask == 0) {
            for (int i = 0; i < n; ++i)
                if (a.units[i].op != OP_PASS) apply_unit(f, i, a.units[i], day);
            return;
        }
        int demand[N_CROPS] = {0};
        if (a.metadata_ready) {
            for (int crop = 0; crop < N_CROPS; ++crop) demand[crop] = a.plant_demand[crop];
        } else {
            for (int i = 0; i < a.n_units; ++i)
                if (a.units[i].op == OP_PLANT && a.units[i].arg < N_CROPS)
                    ++demand[a.units[i].arg];
        }
        bool blocked[N_CROPS];
        for (int crop = 0; crop < N_CROPS; ++crop) blocked[crop] = demand[crop] > f.seeds[crop];

        for (int i = 0; i < n; ++i) {
            const UnitAction& u = a.units[i];
            if (u.op == OP_PLANT && u.arg < N_CROPS && blocked[u.arg]) continue;
            if (u.op != OP_PASS) apply_unit(f, i, u, day);
        }
    }

    void apply_unit(Farm& f, int idx, const UnitAction& u, int day) {
        const int bs = cfg.board_size;
        int fx = f.pos_x[idx], fy = f.pos_y[idx];
        Count* inv = f.inv[idx];

        switch (u.op) {
            case OP_PASS: return;
            case OP_NORTH: case OP_SOUTH: case OP_EAST: case OP_WEST: {
                int nx = fx + (u.op == OP_EAST) - (u.op == OP_WEST);
                int ny = fy + (u.op == OP_SOUTH) - (u.op == OP_NORTH);
                // Movement onto LOCKED tiles is legal (hands can spawn there).
                if (nx < 0 || nx >= bs || ny < 0 || ny >= bs) return;
                f.pos_x[idx] = static_cast<int8_t>(nx);
                f.pos_y[idx] = static_cast<int8_t>(ny);
                return;
            }
            default: break;
        }

        Tile& tile = f.tiles[fy][fx];

        // Shed ops resolve before the LOCKED guard: three of the four
        // shed-access tiles start locked, and the shed itself is always owned.
        if (u.op == OP_DROP) {
            if (!is_shed_adjacent(fx, fy, bs)) return;
            // Insertion order, matching `for item, n in list(inv.items())`.
            uint8_t keys[N_ITEMS];
            int nk = f.inv_nkeys[idx];
            for (int k = 0; k < nk; ++k) keys[k] = f.inv_keys[idx][k];
            for (int k = 0; k < nk; ++k) {
                int it = keys[k];
                if (inv[it] <= 0) { f.inv_erase(idx, it); continue; }
                int room = std::max(0, cfg.shed_capacity - f.shed_total);
                int take = std::min<int>(inv[it], room);
                if (take > 0) { f.shed[it] += static_cast<Count>(take); f.shed_total += take; }
                f.discarded[it] += inv[it] - take;
                f.inv_erase(idx, it);
            }
            return;
        }
        if (u.op == OP_PICKUP) {
            if (!is_shed_adjacent(fx, fy, bs)) return;
            if (u.n <= 0 || u.arg >= N_ITEMS) return;
            int take = std::min<int>(u.n, f.shed[u.arg]);
            if (take <= 0) return;
            f.shed[u.arg] -= static_cast<Count>(take); f.shed_total -= take;
            f.inv_add(idx, u.arg, take);
            return;
        }
        if (u.op == OP_PLACE) {
            if (u.arg >= N_ITEMS) return;
            if (is_animal(u.arg)) {
                const AnimalDef& ad = ANIMALS[u.arg - GOOSE];
                TileKind want = (ad.structure == ST_COOP) ? T_COOP : T_PASTURE;
                if (tile.kind == want && !tile.has_animal) {
                    if (f.inv_take(idx, u.arg, 1)) {
                        Tile t{};
                        t.kind = want; t.what = u.arg; t.has_animal = true;
                        t.planted_day = static_cast<int16_t>(day);
                        tile = t;
                        set_mask(f.animal_mask, fx, fy, true);
                    }
                    return;
                }
            }
            if (is_shed_adjacent(fx, fy, bs)) {
                int n = std::min<int>(u.n, inv[u.arg]);
                if (n <= 0) return;
                n = std::min(n, std::max(0, cfg.shed_capacity - f.shed_total));
                if (n <= 0) return;
                f.inv_take(idx, u.arg, n);
                f.shed[u.arg] += static_cast<Count>(n); f.shed_total += n;
            }
            return;
        }

        if (tile.kind == T_LOCKED) return;

        switch (u.op) {
            case OP_PLANT: {
                if (u.arg >= N_CROPS || tile.kind != T_EMPTY || f.seeds[u.arg] <= 0) return;
                f.seeds[u.arg] -= 1;
                const CropDef& cd = CROPS[u.arg];
                Tile t{};
                t.kind = T_PLANT; t.what = u.arg;
                t.planted_day = static_cast<int16_t>(day);
                t.consecutive_dry = 1;                 // planting day counts as unwatered
                t.yield_units = cd.ongoing ? 0 : 1;
                t.max_lifespan_step = cd.ongoing ? -1
                    : (day + cd.max_yield_day + 1) * cfg.turns_per_day;
                tile = t;
                set_mask(f.empty_mask, fx, fy, false);
                set_mask(f.plant_mask, fx, fy, true);
                set_decay(f, fx, fy, !cd.ongoing);
                return;
            }
            case OP_WATER: {
                if (tile.kind != T_PLANT || tile.watered_today) return;
                tile.watered_today = true;
                const CropDef& cd = CROPS[tile.what];
                if (!cd.ongoing) {
                    int age = day - tile.planted_day;
                    int w0 = (cd.max_yield_day + 1) / 2;
                    if (age >= w0 && age <= cd.max_yield_day) {
                        int bonus = (tile.fertilized_until_day >= day) ? 2 : 1;
                        tile.yield_units = static_cast<int8_t>(std::min(cd.max_yield, tile.yield_units + bonus));
                    }
                }
                return;
            }
            case OP_HARVEST: {
                if (tile.kind == T_EMPTY || tile.kind == T_WEED) return;
                if (tile.yield_units <= 0) return;
                if (tile.kind == T_PLANT) {
                    const CropDef& cd = CROPS[tile.what];
                    if (day - tile.planted_day < cd.first_yield_day) return;
                    f.inv_add(idx, tile.what, tile.yield_units);
                    f.produced[tile.what] += tile.yield_units;
                    tile.yield_units = 0;
                    if (!cd.ongoing) {
                        Tile e{}; e.kind = T_EMPTY; tile = e;
                        set_mask(f.plant_mask, fx, fy, false);
                        set_mask(f.empty_mask, fx, fy, true);
                        set_decay(f, fx, fy, false);
                    }
                } else if (tile.has_animal) {
                    f.inv_add(idx, ANIMALS[tile.what - GOOSE].product, tile.yield_units);
                    f.produced[ANIMALS[tile.what - GOOSE].product] += tile.yield_units;
                    tile.yield_units = 0;
                }
                return;
            }
            case OP_FERTILIZE: {
                if (tile.kind != T_PLANT) return;
                if (!f.inv_take(idx, FERTILIZER, 1)) return;
                tile.fertilized_until_day = std::max<int16_t>(tile.fertilized_until_day,
                                                             static_cast<int16_t>(day + 2));
                return;
            }
            case OP_DIG: {
                if (tile.kind == T_EMPTY) return;
                if (tile.has_animal) return;           // animals are not removable
                Tile e{}; e.kind = T_EMPTY; tile = e;
                set_mask(f.plant_mask, fx, fy, false);
                set_mask(f.empty_mask, fx, fy, true);
                set_decay(f, fx, fy, false);
                return;
            }
            case OP_BUILD_COOP:
                if (tile.kind != T_EMPTY) return;
                { Tile t{}; t.kind = T_COOP; tile = t; }
                set_mask(f.empty_mask, fx, fy, false);
                return;
            case OP_BUILD_PASTURE:
                if (tile.kind != T_EMPTY) return;
                { Tile t{}; t.kind = T_PASTURE; tile = t; }
                set_mask(f.empty_mask, fx, fy, false);
                return;
            case OP_FEED:
                if (!tile.has_animal || tile.fed_today) return;
                if (!f.inv_take(idx, WHEAT, 1)) return;
                tile.fed_today = true;
                return;
            case OP_COLLECT_FERTILIZER:
                if (!tile.has_animal || !tile.fertilizer_available) return;
                tile.fertilizer_available = false;
                f.inv_add(idx, FERTILIZER, 1);
                f.produced[FERTILIZER] += 1;
                return;
            case OP_CARE:
                if (!tile.has_animal || tile.cared_today) return;
                tile.cared_today = true;
                return;
            default: return;
        }
    }

    // ------------------------------------------------------------ market
    struct OState { uint8_t type; uint8_t item; int32_t remaining; bool live; };

    void process_market(const Action& a0, const Action& a1) {
        const Action* acts[2] = { &a0, &a1 };
        int nq[2];
        for (int p = 0; p < 2; ++p) nq[p] = std::min(acts[p]->n_orders, cfg.max_orders);
        int max_len = std::max(nq[0], nq[1]);

        for (int i = 0; i < max_len; ++i) {
            OState os[2]{};
            for (int p = 0; p < 2; ++p) {
                os[p].live = false;
                if (i < nq[p]) {
                    const Order& o = acts[p]->orders[i];
                    if (o.op == M_HIRE || o.op == M_BUY_LAND) {
                        os[p].type = o.op; os[p].live = true; os[p].remaining = 1;
                    } else if (o.op != M_NONE && o.n > 0) {
                        os[p].type = o.op; os[p].item = o.item; os[p].remaining = o.n; os[p].live = true;
                    }
                }
            }
            // Atomic orders resolve once, in player order.
            for (int p = 0; p < 2; ++p) {
                if (!os[p].live) continue;
                if (os[p].type == M_HIRE) { do_hire(st.farms[p]); os[p].live = false; }
                else if (os[p].type == M_BUY_LAND) { do_buy_land(st.farms[p]); os[p].live = false; }
            }

            // Per-unit lockstep: both players see the same pre-commit inventory.
            for (;;) {
                struct Q { bool ok = false; uint8_t type = 0; uint8_t item = 0; int price = 0; };
                Q q[2];
                for (int p = 0; p < 2; ++p) {
                    if (!os[p].live || os[p].remaining <= 0) continue;
                    uint8_t t = os[p].type, it = os[p].item;
                    if (t == M_SELL && is_product(it)) {
                        q[p] = { true, t, it, market_price(it, st.market.inventory[it]) };
                    } else if (t == M_BUY_PRODUCT && (it == WHEAT || it == FERTILIZER)) {
                        // Quoted at post-buy inventory so a round trip nets zero.
                        q[p] = { true, t, it, market_price(it, st.market.inventory[it] - 1) };
                    } else if (t == M_BUY_SEED && is_crop(it)) {
                        q[p] = { true, t, it, CROPS[it].seed };
                    } else if (t == M_BUY_ANIMAL && is_animal(it)) {
                        q[p] = { true, t, it, ANIMALS[it - GOOSE].cost };
                    } else {
                        os[p].live = false;
                    }
                }
                if (!q[0].ok && !q[1].ok) break;
                bool committed = false;
                for (int p = 0; p < 2; ++p) {
                    if (!q[p].ok) continue;
                    if (commit_unit(q[p].type, q[p].item, q[p].price, st.farms[p])) {
                        os[p].remaining -= 1; committed = true;
                    } else {
                        os[p].live = false;
                    }
                }
                if (!committed) break;
            }
            refresh_prices();
        }
    }

    bool commit_unit(uint8_t op, uint8_t item, int price, Farm& f) {
        switch (op) {
            case M_SELL:
                if (f.shed[item] <= 0) return false;
                f.shed[item] -= 1; f.shed_total -= 1;
                f.money += price;
                f.sold_units[item] += 1;
                f.sell_revenue += price;
                if (price > 1) {
                    st.market.inventory[item] += 1;              // $1 sales don't add supply
                    mark_price_dirty(item);
                }
                return true;
            case M_BUY_PRODUCT:
                if (f.money < price) return false;
                if (f.shed_total >= cfg.shed_capacity) return false;
                f.money -= price; f.total_spend += price;
                f.shed[item] += 1; f.shed_total += 1;
                st.market.inventory[item] -= 1;
                mark_price_dirty(item);
                return true;
            case M_BUY_SEED:
                if (f.money < price) return false;
                f.money -= price; f.total_spend += price; f.seeds[item] += 1;
                return true;
            case M_BUY_ANIMAL:
                if (f.money < price) return false;
                if (f.shed_total >= cfg.shed_capacity) return false;
                f.money -= price; f.total_spend += price;
                f.shed[item] += 1; f.shed_total += 1;
                return true;
        }
        return false;
    }

    void refresh_prices() {
#ifdef KAG_DISABLE_DIRTY_PRICES
        for (int item = 0; item < N_PRODUCTS; ++item)
            st.market.prices[item] = market_price(item, st.market.inventory[item]);
        dirty_products_ = 0;
#else
        uint16_t dirty = dirty_products_;
        while (dirty) {
            const int item = std::countr_zero(dirty);
            st.market.prices[item] = market_price(item, st.market.inventory[item]);
            dirty &= dirty - 1;
        }
        dirty_products_ = 0;
#endif
    }

    void do_hire(Farm& f) {
        int cost = cfg.hire_mult * fib(f.hires_today);
        if (f.money < cost || f.n_units >= MAX_UNITS) return;
        f.money -= cost; f.total_spend += cost;
        f.hires_today += 1;
        // Spawn on the first free shed-access tile (NWSE), ties by occupancy.
        int acc[4][2]; shed_access_tiles(cfg.board_size, acc);
        int occ[4] = {0,0,0,0};
        for (int u = 0; u < f.n_units; ++u)
            for (int k = 0; k < 4; ++k)
                if (f.pos_x[u] == acc[k][0] && f.pos_y[u] == acc[k][1]) occ[k]++;
        int best = 0;
        for (int k = 1; k < 4; ++k) if (occ[k] < occ[best]) best = k;
        int idx = f.n_units++;
        f.pos_x[idx] = static_cast<int8_t>(acc[best][0]);
        f.pos_y[idx] = static_cast<int8_t>(acc[best][1]);
        f.inv_clear(idx);
    }

    void do_buy_land(Farm& f) {
        int extra = f.n_quadrants - 1;
        if (extra >= 3) return;
        int cost = LAND_PRICES[extra];
        if (f.money < cost) return;
        f.money -= cost; f.total_spend += cost;
        int quad = LAND_ORDER[extra];
        f.n_quadrants += 1;
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x)
                if (quadrant_of(x, y, cfg.board_size) == quad && f.tiles[y][x].kind == T_LOCKED)
                    f.tiles[y][x].kind = T_EMPTY, set_mask(f.empty_mask, x, y, true);
    }

    // ------------------------------------------------------------ town / decay
    void town_consume(int step_i) {
        if (step_i == next_shop_sell_step_) {
            next_shop_sell_step_ += cfg.shop_sell_interval;
            for (int s = 0; s < st.n_shops; ++s) {
                uint16_t mask = SHOP_MASK[st.shops[s]];
                int mult = SHOP_MULT[st.shops[s]];
                for (int it = 0; it < N_PRODUCTS; ++it)
                    if (mask & (1u << it)) {
                        st.market.inventory[it] -= mult;
                        mark_price_dirty(it);
                    }
            }
        }
        if (step_i == next_center_sell_step_) {
            next_center_sell_step_ += cfg.center_sell_interval;
#ifdef KAG_EXPLICIT_AVX2
            const __m256i inventory = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(st.market.inventory));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(st.market.inventory),
                               _mm256_sub_epi32(inventory, _mm256_set1_epi32(1)));
#else
            for (int it = 0; it < FERTILIZER; ++it) st.market.inventory[it] -= 1;
#endif
            dirty_products_ |= (uint16_t{1} << FERTILIZER) - 1;
        }
        refresh_prices();
    }

    void decay_plants(Farm& f, int step_i) {
#ifdef KAG_DISABLE_DECAY_MASK
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x) {
                Tile& tile = f.tiles[y][x];
                if (tile.kind != T_PLANT) continue;
                if (tile.max_lifespan_step < 0 || step_i < tile.max_lifespan_step) continue;
                if ((step_i - tile.max_lifespan_step) & 1) continue;
                --tile.yield_units;
                if (tile.yield_units <= 0) {
                    Tile weed{}; weed.kind = T_WEED; tile = weed;
                    set_mask(f.plant_mask, x, y, false);
                    set_decay(f, x, y, false);
                }
            }
#else
#ifndef KAG_DISABLE_DECAY_SCHEDULER
        if (step_i < f.next_decay_step) return;
        int next_decay_step = std::numeric_limits<int>::max();
#endif
        for (int word = 0; word < 2; ++word) {
            uint64_t active = f.decay_mask[word];
            while (active) {
                const int bit = std::countr_zero(active);
                const int index = word * 64 + bit;
                const int y = index / BOARD;
                const int x = index % BOARD;
                Tile& tile = f.tiles[y][x];
                if (tile.kind != T_PLANT || tile.max_lifespan_step < 0) {
                    f.decay_mask[word] &= ~(uint64_t{1} << bit);
                } else if (step_i >= tile.max_lifespan_step &&
                           ((step_i - tile.max_lifespan_step) & 1) == 0) {
                    --tile.yield_units;
                    if (tile.yield_units <= 0) {
                        Tile weed{}; weed.kind = T_WEED; tile = weed;
                        set_mask(f.plant_mask, x, y, false);
                        f.decay_mask[word] &= ~(uint64_t{1} << bit);
                    }
                }
#ifndef KAG_DISABLE_DECAY_SCHEDULER
                if (tile.kind == T_PLANT && tile.max_lifespan_step >= 0) {
                    int candidate = tile.max_lifespan_step;
                    if (candidate <= step_i)
                        candidate += ((step_i - candidate) / 2 + 1) * 2;
                    next_decay_step = std::min(next_decay_step, candidate);
                }
#endif
                active &= active - 1;
            }
        }
#ifndef KAG_DISABLE_DECAY_SCHEDULER
        f.next_decay_step = next_decay_step;
#endif
#endif
    }

    void end_of_day(int day) {
        PyRandom rng((cfg.seed * 1000003ull) ^ static_cast<uint64_t>(day));
        for (int p = 0; p < 2; ++p) {
            Farm& f = st.farms[p];
            daily_refresh_plants(f, day);
            daily_refresh_animals(f, day);
            spawn_weeds(f, rng);
            drop_inventories(f);
            int h = cfg.board_size / 2;
            int previous_units = f.n_units;
            f.n_units = 1;
            f.pos_x[0] = static_cast<int8_t>(h - 1);
            f.pos_y[0] = static_cast<int8_t>(h - 1);
            f.hires_today = 0;
            for (int u = 0; u < previous_units; ++u) f.inv_clear(u);
        }
        int next_day = day + 1;
        if (next_day == next_shop_unlock_day_ && st.n_shops < MAX_SHOP_INSTANCES) {
            st.shops[st.n_shops++] = static_cast<uint8_t>(rng.choice_index(N_SHOPS));
            next_shop_unlock_day_ += cfg.shop_unlock_interval;
        }
    }

    void daily_refresh_plants(Farm& f, int day) {
        int next_day = day + 1;
#ifdef KAG_DISABLE_ENTITY_MASKS
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x) {
                Tile& t = f.tiles[y][x];
                if (t.kind != T_PLANT) continue;
#else
        for (int word = 0; word < 2; ++word) {
            uint64_t active = f.plant_mask[word];
            while (active) {
                const int bit = std::countr_zero(active);
                const int index = word * 64 + bit;
                const int y = index / BOARD;
                const int x = index % BOARD;
                Tile& t = f.tiles[y][x];
                active &= active - 1;
#endif
                bool was_watered = t.watered_today;
                t.consecutive_dry = was_watered ? 0 : static_cast<int8_t>(t.consecutive_dry + 1);
                t.watered_today = false;
                if (t.consecutive_dry >= 2) {
                    Tile w{}; w.kind = T_WEED; t = w;
                    set_mask(f.plant_mask, x, y, false);
                    set_decay(f, x, y, false);
                    continue;
                }
                const CropDef& cd = CROPS[t.what];
                if (!cd.ongoing) continue;
                int since = next_day - t.planted_day - cd.first_yield_day;
                if (since < 0) continue;
                if (since % cd.interval != 0) continue;
                int count = since / cd.interval + 1;
                if (count > cd.max_yield) continue;
                bool fert = was_watered && t.fertilized_until_day >= day;
                t.yield_units = static_cast<int8_t>(std::min(cd.max_yield, t.yield_units + (fert ? 2 : 1)));
                if (count == cd.max_yield) {
                    t.max_lifespan_step = (next_day + 1) * cfg.turns_per_day;
                    set_decay(f, x, y, true);
                }
            }
#ifndef KAG_DISABLE_ENTITY_MASKS
        }
#endif
    }

    void daily_refresh_animals(Farm& f, int day) {
        int next_day = day + 1;
#ifdef KAG_DISABLE_ENTITY_MASKS
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x) {
                Tile& t = f.tiles[y][x];
                if (!t.has_animal) continue;
#else
        for (int word = 0; word < 2; ++word) {
            uint64_t active = f.animal_mask[word];
            while (active) {
                const int bit = std::countr_zero(active);
                const int index = word * 64 + bit;
                Tile& t = f.tiles[index / BOARD][index % BOARD];
#endif
                t.consecutive_dry = t.fed_today ? 0 : static_cast<int8_t>(t.consecutive_dry + 1);
                if (t.consecutive_dry >= 2) {           // escapes; structure remains
                    TileKind k = t.kind;
                    Tile s{}; s.kind = k; t = s;
#ifndef KAG_DISABLE_ENTITY_MASKS
                    f.animal_mask[word] &= ~(uint64_t{1} << bit);
                    active &= active - 1;
#endif
                    continue;
                }
                const AnimalDef& ad = ANIMALS[t.what - GOOSE];
                int since = next_day - t.planted_day - ad.first_yield_day;
                if (since >= 0 && since % ad.interval == 0) {
                    int bonus = t.fed_today ? t.pending_care_bonus : 0;
                    t.yield_units = static_cast<int8_t>(std::min(ad.max_held, t.yield_units + 1 + bonus));
                    t.pending_care_bonus = 0;
                }
                if (t.cared_today && t.fed_today) t.pending_care_bonus += 1;
                t.fertilizer_available = true;
                t.fed_today = false;
                t.cared_today = false;
#ifndef KAG_DISABLE_ENTITY_MASKS
                active &= active - 1;
#endif
            }
#ifndef KAG_DISABLE_ENTITY_MASKS
        }
#endif
    }

    void spawn_weeds(Farm& f, PyRandom& rng) {
#ifdef KAG_VERIFY_MASKS
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x) {
                const int index = tile_index(x, y);
                const uint64_t bit = uint64_t{1} << (index & 63);
                const bool empty_bit = f.empty_mask[index >> 6] & bit;
                const bool plant_bit = f.plant_mask[index >> 6] & bit;
                const bool animal_bit = f.animal_mask[index >> 6] & bit;
                if (empty_bit != (f.tiles[y][x].kind == T_EMPTY) ||
                    plant_bit != (f.tiles[y][x].kind == T_PLANT) ||
                    animal_bit != f.tiles[y][x].has_animal)
                    std::abort();
            }
#endif
#ifdef KAG_DISABLE_ENTITY_MASKS
        for (int y = 0; y < cfg.board_size; ++y)
            for (int x = 0; x < cfg.board_size; ++x)
                if (f.tiles[y][x].kind == T_EMPTY && rng.random() < cfg.weed_chance) {
                    f.tiles[y][x].kind = T_WEED;
                    set_mask(f.empty_mask, x, y, false);
                }
#else
        for (int word = 0; word < 2; ++word) {
            uint64_t active = f.empty_mask[word];
            while (active) {
                const int bit = std::countr_zero(active);
                if (rng.random() < cfg.weed_chance) {
                    const int index = word * 64 + bit;
                    f.tiles[index / BOARD][index % BOARD].kind = T_WEED;
                    f.empty_mask[word] &= ~(uint64_t{1} << bit);
                }
                active &= active - 1;
            }
        }
#endif
    }

    void drop_inventories(Farm& f) {
        // Insertion order again: with the shed near capacity this decides which
        // goods survive the day and which are discarded.
        for (int u = 0; u < f.n_units; ++u) {
            uint8_t keys[N_ITEMS];
            int nk = f.inv_nkeys[u];
            for (int k = 0; k < nk; ++k) keys[k] = f.inv_keys[u][k];
            for (int k = 0; k < nk; ++k) {
                int it = keys[k];
                if (f.inv[u][it] <= 0) { f.inv_erase(u, it); continue; }
                int room = std::max(0, cfg.shed_capacity - f.shed_total);
                int take = std::min<int>(f.inv[u][it], room);
                if (take > 0) { f.shed[it] += static_cast<Count>(take); f.shed_total += take; }
                f.discarded[it] += f.inv[u][it] - take;
                f.inv_erase(u, it);
            }
        }
    }
};

}  // namespace kag
