#include "dc11/router.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <numeric>
#include <type_traits>

namespace dc11 {
// DC12_ROUTEPROF probe: ms per local-search pass (sync, relocate, swap, tails, reverse, member, shed, entity), rounds [8],
// route_day calls [9], ms in route_day [10].
thread_local double route_prof[16]{};
namespace {
const bool route_prof_on = std::getenv("DC12_ROUTEPROF") != nullptr;

constexpr int MAX_ROUTE = 96;
constexpr double LATE_TURN = 50;   // per turn beyond the day, on top of the unserved stops' value
constexpr double SHORTAGE = 5000;  // per carried item needed without a shed stop before it

int shed_tile_near(int from, int to) {
    int best = SHED_TILES[0], score = INT_MAX;
    for (int s : SHED_TILES) {
        const int d = dist(from, s) + (to >= 0 ? dist(s, to) : 0);
        if (d < score) score = d, best = s;
    }
    return best;
}

double wages(int hires) {
    double w = 0;
    for (int k = 0; k < hires; ++k) w += fib(k);
    return w;
}

struct Eval {
    double cost = 0, gain = 0, decay = 0, unserved = 0;
    int end = 0, moves = 0, actions = 0, waits = 0;
    int served = 0;  // stops (prefix) that end within the day
    int required_end = 0;  // hour after the last non-optional tile stop (Problem::optional_late)
    int night = 0, night_wheat = 0;  // units carried after the last action (tonight's deposit)
};

struct NoEmit {
    void operator()(int, UnitAction) const {}
    void deposit(int, int) const {}
};

// Walk state before stop k of a route, a shed stop after a tile stop: every lookahead of the
// earlier stops ends before k, so a route that keeps stops 0..k resumes here with the same result.
struct Resume {
    Eval e;
    int k = 0, pos = 0, t = 0, start_cargo = 0, harvested_wheat = 0, collected = 0;
    bool in_day = true, lazy_cw = false;
    int carry[N_CARRY]{}, cargo[N_PRODUCTS]{};
};
constexpr int MAX_RESUME = 24;
struct Trace {  // resume points of a route's last full walk (Route::stamp)
    uint64_t stamp = 0;
    int count = 0;
    Resume at[MAX_RESUME];
};

// Walks one route: times, pickups, deposits, decay; `emit(hour, action)` receives every action.
// `from`: start at a resume point of the same route start; `record`: store the resume points.
template <class Emit>
Eval walk(const Problem& pb, const std::vector<Stop>& stops, const Worker& w, const int16_t* ids, int n, Emit&& emit,
          const Resume* from = nullptr, Trace* record = nullptr) {
    constexpr bool quiet = std::is_same_v<std::decay_t<Emit>, NoEmit>;
    Eval e;
    const int last_end = pb.last_hour + 1;
    int pos = w.tile, t = w.hour;
    int carry[N_CARRY], cargo[N_PRODUCTS];
    for (int i = 0; i < N_CARRY; ++i) carry[i] = w.carry[i];
    for (int p = 0; p < N_PRODUCTS; ++p) cargo[p] = 0;  // start cargo earns no deposit gain
    int start_cargo = 0;
    for (int p = 0; p < N_PRODUCTS; ++p) start_cargo += w.cargo[p];
    auto act = [&](UnitAction a) {
        emit(t, a);
        ++t;
    };
    auto move_to = [&](int to) {
        if constexpr (quiet) {
            const int d = dist(pos, to);
            t += d, e.moves += d, pos = to;
            return;
        }
        while (pos != to) {
            const int x = pos % BOARD, y = pos / BOARD, tx = to % BOARD, ty = to / BOARD;
            if (x != tx) act({uint8_t(x < tx ? OP_EAST : OP_WEST), 0, 1}), pos += x < tx ? 1 : -1;
            else act({uint8_t(y < ty ? OP_SOUTH : OP_NORTH), 0, 1}), pos += y < ty ? BOARD : -BOARD;
            ++e.moves;
        }
    };
    bool lazy_cw = false;  // lazy_wheat: this trip's wheat is fetched at its first stop that needs it
    auto pickups = [&](int k) {
        int need[N_CARRY]{}, cum[N_CARRY]{};
        for (; k < n && !stops[ids[k]].shed; ++k) {
            const Stop& s = stops[ids[k]];
            for (int i = 0; i < N_CARRY; ++i) {
                need[i] = std::max(need[i], cum[i] + s.peak[i]);
                cum[i] += s.net[i];
            }
        }
        if (!is_shed_tile(pos)) {
            for (int i = 0; i < N_CARRY; ++i) e.cost += SHORTAGE * std::max(0, need[i] - carry[i]);
            return;
        }
        int ready = 0;  // bought units are usable from the hour after their purchase
        lazy_cw = false;
        for (int i = 0; i < N_CARRY; ++i)
            if (need[i] > carry[i] && need[i] - carry[i] > (w.hour == 0 ? pb.dawn_stock[i] : 0)) {
                if (i == CW && pb.lazy_wheat && pb.buy_from[CW] > t) {
                    lazy_cw = true;
                    continue;
                }
                ready = std::max<int>(ready, pb.buy_from[i]);
            }
        if constexpr (quiet) {
            if (t < ready) e.waits += ready - t, t = ready;
        } else {
            while (t < ready) act({}), ++e.waits;
        }
        for (int i = 0; i < N_CARRY; ++i)
            if (need[i] > carry[i] && !(i == CW && lazy_cw)) {
                if (i == CW) e.cost += pb.wheat_buy_cost * (need[i] - carry[i]);
                act({OP_PICKUP, CARRY_ITEM[i], need[i] - carry[i]});
                ++e.actions;
                carry[i] = need[i];
            }
    };
    int harvested_wheat = 0, collected = 0;  // cargo parts of the carried consumables
    bool in_day = true;
    int first = 0;
    if (from) {
        e = from->e, first = from->k, pos = from->pos, t = from->t;
        start_cargo = from->start_cargo, harvested_wheat = from->harvested_wheat, collected = from->collected;
        in_day = from->in_day, lazy_cw = from->lazy_cw;
        std::copy_n(from->carry, N_CARRY, carry);
        std::copy_n(from->cargo, N_PRODUCTS, cargo);
    } else {
        pickups(0);
    }
    for (int k = first; k < n; ++k) {
        const Stop& s = stops[ids[k]];
        if (s.shed) {
            if (record && k > 0 && !stops[ids[k - 1]].shed && record->count < MAX_RESUME) {
                Resume& r = record->at[record->count++];
                r.e = e, r.k = k, r.pos = pos, r.t = t;
                r.start_cargo = start_cargo, r.harvested_wheat = harvested_wheat, r.collected = collected;
                r.in_day = in_day, r.lazy_cw = lazy_cw;
                std::copy_n(carry, N_CARRY, r.carry);
                std::copy_n(cargo, N_PRODUCTS, r.cargo);
            }
            int next = -1;
            for (int j = k + 1; j < n && next < 0; ++j)
                if (!stops[ids[j]].shed) next = stops[ids[j]].tile;
            move_to(shed_tile_near(pos, next));
            harvested_wheat = std::clamp(carry[CW], 0, harvested_wheat);  // what feeding left of it
            collected = std::clamp(carry[CF], 0, collected);              // what fertilizing left of it
            int units = harvested_wheat + collected + start_cargo;
            for (int p = 0; p < N_PRODUCTS; ++p) units += cargo[p];
            if (pb.place_deposits && !start_cargo) {  // regime M: PLACE the products, keep collected fertilizer and supplies
                int placed = 0;
                double g = 0;
                for (int p = 0; p < N_PRODUCTS; ++p)
                    if (cargo[p] > 0) {
                        act({OP_PLACE, uint8_t(p), cargo[p]});
                        ++e.actions;
                        if (t <= pb.last_hour) g += cargo[p] * pb.gain[p][t];
                        placed += cargo[p];
                        cargo[p] = 0;
                    }
                if (harvested_wheat > 0) {
                    act({OP_PLACE, uint8_t(WHEAT), harvested_wheat});
                    ++e.actions;
                    if (t <= pb.last_hour) g += harvested_wheat * pb.gain[WHEAT][t];
                    placed += harvested_wheat;
                    carry[CW] -= harvested_wheat;
                    harvested_wheat = 0;
                }
                if (placed > 0) emit.deposit(t, placed), e.gain += g;
            } else if (units > 0) {
                if (t <= pb.last_hour) {
                    double g = harvested_wheat * pb.gain[WHEAT][t] + collected * pb.gain[FERTILIZER][t];
                    for (int p = 0; p < N_PRODUCTS; ++p) g += cargo[p] * pb.gain[p][t];
                    e.gain += g;
                }
                emit.deposit(t, units);
                act({OP_DROP, 0, 1});
                ++e.actions;
                std::fill_n(cargo, N_PRODUCTS, 0);
                std::fill_n(carry, N_CARRY, 0);
                harvested_wheat = collected = start_cargo = 0;
            }
            pickups(k + 1);
        } else {
            if (lazy_cw && s.peak[CW] > carry[CW]) {  // lazy_wheat: to the shed for this trip's wheat, once it is buyable
                int need = 0, cum = 0;
                for (int j = k; j < n && !stops[ids[j]].shed; ++j) {
                    need = std::max(need, cum + stops[ids[j]].peak[CW]);
                    cum += stops[ids[j]].net[CW];
                }
                move_to(shed_tile_near(pos, s.tile));
                if constexpr (quiet) {
                    if (t < pb.buy_from[CW]) e.waits += pb.buy_from[CW] - t, t = pb.buy_from[CW];
                } else {
                    while (t < pb.buy_from[CW]) act({}), ++e.waits;
                }
                e.cost += pb.wheat_buy_cost * (need - carry[CW]);
                act({OP_PICKUP, CARRY_ITEM[CW], need - carry[CW]});
                ++e.actions;
                carry[CW] = need;
                lazy_cw = false;
            }
            move_to(s.tile);
            int start = std::max(t, int(s.release));
            if (s.plant_crop >= 0 && pb.dawn_seeds[s.plant_crop] < pb.plants[s.plant_crop]) start = std::max<int>(start, pb.seed_from[s.plant_crop]);
            if (s.animal && pb.animal_morning > 0 && start > 11) e.cost += pb.animal_morning * (start - 11);  // regime M: morning block
            if constexpr (quiet) {
                if (t < start) e.waits += start - t, t = start;
            } else {
                while (t < start) act({}), ++e.waits;
            }
            int units = s.units;
            if (s.decay_from >= 0 && s.product >= 0) {
                const int h = start + s.harvest_at;
                const int lost = h > s.decay_from ? std::min<int>(units, (h - s.decay_from + 1) / 2) : 0;
                units -= lost;
                e.decay += lost * s.unit_value;
            }
            // A crop whose stored yield decayed to nothing is a weed: dig it instead of the crop work.
            const bool gone = s.decay_from >= 0 && s.product >= 0 && s.units > 0 && units == 0;
            for (int j = 0; j < s.n; ++j) {
                Step st = s.steps[j];
                if (gone && j < s.base_n) {
                    if (st.op != OP_HARVEST) continue;
                    st = {OP_DIG, 0};
                }
                act({st.op, st.arg, 1});
                ++e.actions;
            }
            for (int i = 0; i < N_CARRY; ++i) carry[i] -= s.net[i];
            if (s.product == WHEAT) {
                carry[CW] -= s.units - units;  // net counted the undecayed yield
                harvested_wheat += units;
            } else if (s.product >= 0) {
                cargo[s.product] += units;
            }
            collected += s.fertilizer;
        }
        if (!s.shed && s.priority < P_EXTRA) e.required_end = t;
        if (t > last_end) in_day = false;
        if (in_day) e.served = k + 1;
        else if (!s.shed) e.unserved += PRIORITY_COST[s.priority] + s.value;
    }
    // Final return: the cargo goes to the shed before the last market when that pays for the trip.
    if (in_day && pb.final_return) {
        harvested_wheat = std::clamp(carry[CW], 0, harvested_wheat);
        collected = std::clamp(carry[CF], 0, collected);
        int units = harvested_wheat + collected + start_cargo;
        for (int p = 0; p < N_PRODUCTS; ++p) units += cargo[p];
        const int at = shed_tile_near(pos, -1), arrive = t + dist(pos, at);
        if (units > 0 && arrive <= pb.last_hour) {
            double g = harvested_wheat * pb.gain[WHEAT][arrive] + collected * pb.gain[FERTILIZER][arrive];
            for (int p = 0; p < N_PRODUCTS; ++p) g += cargo[p] * pb.gain[p][arrive];
            if (g > pb.return_cost * pb.wage_per_turn * (dist(pos, at) + 1)) {
                move_to(at);
                emit.deposit(t, units);
                act({OP_DROP, 0, 1});
                ++e.actions;
                e.gain += g;
                std::fill_n(cargo, N_PRODUCTS, 0);
                std::fill_n(carry, N_CARRY, 0);
                start_cargo = 0;
            }
        }
    }
    for (int i = 0; i < N_CARRY; ++i) e.night += std::max(0, carry[i]);
    for (int p = 0; p < N_PRODUCTS; ++p) e.night += cargo[p];
    e.night += start_cargo;
    e.night_wheat = std::max(0, carry[CW]);
    e.end = t;
    const int span = std::max(0, std::min(t, last_end) - w.hour);
    e.cost += pb.wage_per_turn * span + LATE_TURN * std::max(0, (pb.optional_late ? e.required_end : t) - last_end) + e.decay - e.gain +
              e.unserved;
    return e;
}

struct Route {
    int16_t ids[MAX_ROUTE];
    int n = 0;
    Worker start;
    double cost = 0;
    int end = 0, required_end = 0;
    uint64_t stamp = 0;  // last full walk (its Trace); edits in place keep it until route_cost
};

struct State {
    std::vector<Stop> stops;
    std::vector<Entity> ents;
    std::vector<Route> routes;
    std::vector<int> where;          // stop -> route (-1 unrouted)
    std::vector<uint8_t> site_used;  // tile used by an entity's own stop
};

struct Search {
    const Problem& pb;
    State st;
    long evaluations = 0;
    int shed_probe = -1;  // a shed stop used for trial evaluations
    // Per route index: resume points of its last two route_costs (a trial edit that is undone, as in
    // relocate_pass, brings back the previous stamp).
    std::vector<std::array<Trace, 2>> traces;
    uint64_t stamps = 0;
    // Exact skips of repeated work. A scan that changed nothing repeats with the same result while the
    // routes it read keep their stamps; a skip still counts the scan's evaluations (same work budget).
    struct PairMemo { uint64_t a = 0, b = 0; long evaluations = 0; };
    std::vector<PairMemo> tails_memo = std::vector<PairMemo>(MAX_WORKERS * MAX_WORKERS);  // route pair x * MAX_WORKERS + y
    std::vector<PairMemo> reverse_memo = std::vector<PairMemo>(MAX_WORKERS);              // route
    std::vector<PairMemo> swap_memo;     // stop s * MAX_WORKERS + route: swaps of s with that route's stops without a gain
    // best_insertion of a stop into a route: the exact deltas of the positions it tried, in order.
    static constexpr int MAX_KEEP = 6;  // best_insertion's keep_positions
    struct InsertMemo { uint64_t stamp = 0; int8_t keep = 0, count = 0; int8_t pos[MAX_KEEP]; double delta[MAX_KEEP]; };
    std::vector<InsertMemo> insert_memo;  // stop * MAX_WORKERS + route
    std::vector<Stop> insert_stop;        // the stop's content when its entries were made

    explicit Search(const Problem& p) : pb(p) { reset(); }

    void reset() {
        st.stops = pb.stops;
        for (auto& s : st.stops) {
            s.base_n = s.n;
            derive(s);
        }
        st.ents = pb.entities;
        st.routes.clear();
        st.where.assign(st.stops.size(), -1);
        st.site_used.assign(BOARD * BOARD, 0);
        shed_probe = new_shed_stop();
        std::fill_n(spawn_at, MAX_WORKERS, int8_t(-1));
    }

    // Route r with stops 0..first-1 unchanged (ids and stop contents) since its last route_cost.
    Eval evaluate(int r, const int16_t* ids, int n, int first) {
        ++evaluations;
        const Route& R = st.routes[r];
        const Resume* from = nullptr;
        if (r < int(traces.size()))
            for (const Trace& trace : traces[r])
                if (trace.stamp == R.stamp)
                    for (int i = trace.count - 1; i >= 0 && !from; --i)
                        if (trace.at[i].k < first) from = &trace.at[i];
        return walk(pb, st.stops, R.start, ids, n, NoEmit{}, from);
    }
    double route_cost(Route& r) {
        ++evaluations;
        const size_t index = &r - st.routes.data();
        if (index >= st.routes.size()) std::abort();
        if (traces.size() < st.routes.size()) traces.resize(st.routes.size());
        Trace& trace = traces[index][traces[index][0].stamp > traces[index][1].stamp];
        trace.count = 0;
        trace.stamp = r.stamp = ++stamps;
        const Eval e = walk(pb, st.stops, r.start, r.ids, r.n, NoEmit{}, nullptr, &trace);
        r.cost = e.cost, r.end = e.end, r.required_end = e.required_end;
        return r.cost;
    }
    double total() const {
        double c = 0;
        for (const auto& r : st.routes) c += r.cost;
        for (int e = 0; e < int(st.ents.size()); ++e) c += entity_site_cost(e);
        // Entities never placed cost as unserved stops.
        for (const auto& en : st.ents)
            if (en.stop < 0) c += PRIORITY_COST[en.priority] + en.value;
        return c;
    }
    // Turns past the end of the day (unplaced entities have no site at any hire count: not late).
    int late() const {
        int n = 0;
        for (const auto& r : st.routes) n += std::max(0, (pb.optional_late ? r.required_end : r.end) - (pb.last_hour + 1));
        return n;
    }

    // ------------------------------------------------------------ stops and entities
    static void derive(Stop& s) {
        std::fill_n(s.net, N_CARRY, 0);
        std::fill_n(s.peak, N_CARRY, 0);
        s.plant_crop = -1;
        int cum[N_CARRY]{};
        for (int k = 0; k < s.n; ++k) {
            const Step st = s.steps[k];
            if (st.op == OP_FEED) ++cum[CW];
            if (st.op == OP_FERTILIZE) ++cum[CF];
            if (st.op == OP_COLLECT_FERTILIZER) --cum[CF];
            if (st.op == OP_PLACE && is_animal(st.arg)) ++cum[CG + st.arg - GOOSE];
            if (st.op == OP_HARVEST && s.product == WHEAT) cum[CW] -= s.units;
            if (st.op == OP_PLANT) s.plant_crop = int8_t(st.arg);
            for (int i = 0; i < N_CARRY; ++i) s.peak[i] = int8_t(std::max<int>(s.peak[i], cum[i]));
        }
        for (int i = 0; i < N_CARRY; ++i) s.net[i] = int8_t(cum[i]);
    }

    // Steps of an entity on a free site (dawn state `site`) or on a tile freed today (nullptr).
    static int entity_steps(const Entity& en, const Tile* site, Step* out) {
        int n = 0;
        const bool animal = is_animal(en.item);
        const TileKind want = animal ? (ANIMALS[en.item - GOOSE].structure == ST_COOP ? T_COOP : T_PASTURE) : T_EMPTY;
        bool build = animal;
        if (site) {
            const bool reuse = animal && site->kind == want && !site->has_animal;
            if (site->kind != T_EMPTY && site->kind != T_LOCKED && !reuse) out[n++] = {OP_DIG, 0};
            build = animal && !reuse;
        }
        if (build) out[n++] = {uint8_t(want == T_COOP ? OP_BUILD_COOP : OP_BUILD_PASTURE), 0};
        if (animal) {
            out[n++] = {OP_PLACE, en.item};
            if (en.extra & 4) out[n++] = {OP_FEED, 0};
            if (en.extra & 8) out[n++] = {OP_CARE, 0};
        } else {
            out[n++] = {OP_PLANT, en.item};
            if (en.extra & 1) out[n++] = {OP_FERTILIZE, 0};
            out[n++] = {OP_WATER, 0};
        }
        return n;
    }

    double site_cost(const Entity& en, int tile) const {
        double c = en.site_weight * pb.wage_per_turn * shed_dist(tile);
        if (!is_animal(en.item) && pb.reserve_animal_sites && shed_dist(tile) <= 1) c += 6 * pb.wage_per_turn;
        return c;
    }
    double entity_site_cost(int e) const {
        const int s = st.ents[e].stop;
        return s >= 0 ? site_cost(st.ents[e], st.stops[s].tile) : 0;
    }

    void attach(int e, int h) {
        Stop& s = st.stops[h];
        Step extra[MAX_STEPS];
        const int m = entity_steps(st.ents[e], nullptr, extra);
        s.n = s.base_n;
        for (int k = 0; k < m; ++k) s.steps[s.n++] = extra[k];
        s.entity = int16_t(e);
        derive(s);
        st.ents[e].stop = int16_t(h);
    }
    void detach(int h) {
        Stop& s = st.stops[h];
        s.n = s.base_n;
        s.entity = -1;
        derive(s);
    }
    int own_stop(int e, int tile) {
        Stop s;
        s.tile = int16_t(tile);
        s.n = s.base_n = uint8_t(entity_steps(st.ents[e], &pb.tiles[tile], s.steps));
        s.base_n = 0;
        s.release = std::max<int8_t>(0, pb.site_release[tile]);
        s.priority = st.ents[e].priority;
        s.value = st.ents[e].value;
        s.entity = int16_t(e);
        derive(s);
        st.stops.push_back(s);
        st.where.push_back(-1);
        st.site_used[tile] = 1;
        st.ents[e].stop = int16_t(st.stops.size() - 1);
        return int(st.stops.size()) - 1;
    }
    bool is_host(int s) const { return st.stops[s].frees && !st.stops[s].dead; }
    int new_shed_stop() {
        Stop s;
        s.shed = true;
        s.priority = P_EXTRA;
        st.stops.push_back(s);
        st.where.push_back(-1);
        return int(st.stops.size()) - 1;
    }

    // ------------------------------------------------------------ route edits
    void insert_at(int r, int pos, int stop) {
        Route& R = st.routes[r];
        for (int k = R.n; k > pos; --k) R.ids[k] = R.ids[k - 1];
        R.ids[pos] = int16_t(stop);
        ++R.n;
        st.where[stop] = r;
    }
    void erase_stop(int stop) {
        Route& R = st.routes[st.where[stop]];
        int pos = 0;
        while (R.ids[pos] != stop) ++pos;
        for (int k = pos; k + 1 < R.n; ++k) R.ids[k] = R.ids[k + 1];
        --R.n;
        st.where[stop] = -1;
    }

    struct Slot { int route = -1, pos = -1; double delta = 1e18; };
    // Best insertion of `stop` over all routes; exact evaluation of the positions with the
    // cheapest detours.
    Slot best_insertion(int stop, int keep_positions = MAX_KEEP) {
        if (keep_positions > MAX_KEEP) std::abort();
        Slot best;
        int16_t buf[MAX_ROUTE];
        const int tile = st.stops[stop].shed ? 44 : st.stops[stop].tile;
        if (insert_stop.size() < st.stops.size()) {
            insert_stop.resize(st.stops.size());
            insert_memo.resize(st.stops.size() * MAX_WORKERS);
        }
        if (std::memcmp(&insert_stop[stop], &st.stops[stop], sizeof(Stop))) {
            std::memcpy(&insert_stop[stop], &st.stops[stop], sizeof(Stop));
            for (int r = 0; r < MAX_WORKERS; ++r) insert_memo[stop * MAX_WORKERS + r].stamp = 0;
        }
        for (int r = 0; r < int(st.routes.size()); ++r) {
            const Route& R = st.routes[r];
            if (R.n + 1 >= MAX_ROUTE) continue;
            InsertMemo& memo = insert_memo[stop * MAX_WORKERS + r];
            if (memo.stamp == R.stamp && memo.keep == keep_positions) {
                evaluations += memo.count;
                for (int q = 0; q < memo.count; ++q)
                    if (memo.delta[q] < best.delta - 1e-9) best = {r, memo.pos[q], memo.delta[q]};
                continue;
            }
            int order[MAX_ROUTE + 1], approx[MAX_ROUTE + 1];
            int prev = R.start.tile;
            for (int p = 0; p <= R.n; ++p) {
                const int next = p < R.n ? (st.stops[R.ids[p]].shed ? 44 : st.stops[R.ids[p]].tile) : -1;
                approx[p] = dist(prev, tile) + (next >= 0 ? dist(tile, next) - dist(prev, next) : 0);
                order[p] = p;
                if (next >= 0) prev = next;
            }
            const int count = R.n + 1, keep = std::min(count, keep_positions);
            std::partial_sort(order, order + keep, order + count, [&](int a, int b) {
                return approx[a] != approx[b] ? approx[a] < approx[b] : a > b;
            });
            memo.stamp = R.stamp, memo.keep = int8_t(keep_positions), memo.count = int8_t(keep);
            for (int q = 0; q < keep; ++q) {
                const int p = order[q];
                std::copy_n(R.ids, p, buf);
                buf[p] = int16_t(stop);
                std::copy_n(R.ids + p, R.n - p, buf + p + 1);
                const double d = evaluate(r, buf, R.n + 1, p).cost - R.cost;
                memo.pos[q] = int8_t(p), memo.delta[q] = d;
                if (d < best.delta - 1e-9) best = {r, p, d};
            }
        }
        return best;
    }

    // ------------------------------------------------------------ construction
    Worker start_of(int w) const {
        if (w == 0) return pb.farmer;
        // Before the first sync: occupancy estimate with every worker on its start tile.
        int positions[MAX_WORKERS], count = 0;
        positions[count++] = pb.farmer.tile;
        Worker hire;
        for (int k = 1; k <= w; ++k) {
            hire.tile = int8_t(spawn_at[k] >= 0 ? spawn_at[k] : spawn_tile(positions, count));
            positions[count++] = hire.tile;
        }
        const int wave = w - 1;  // 0-based index among today's hires
        hire.hour = int8_t(pb.farmer.hour + 1 + (wave >= pb.first_wave ? 1 : 0));
        if (wave >= pb.cash_wave) hire.hour = int8_t(std::max<int>(hire.hour, pb.cash_hire_hour + 1));  // latehire
        return hire;
    }
    void add_route(int w) {
        Route r;
        r.start = start_of(w);
        st.routes.push_back(r);
        route_cost(st.routes.back());
    }

    void construct(int workers) {
        for (int w = 0; w < workers; ++w) add_route(w);
        std::vector<int> order;
        for (int s = 0; s < int(st.stops.size()); ++s)
            if (!st.stops[s].shed && !st.stops[s].dead && st.stops[s].n) order.push_back(s);
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            const Stop &x = st.stops[a], &y = st.stops[b];
            if (x.priority != y.priority) return x.priority < y.priority;
            if (x.release != y.release) return x.release > y.release;
            return shed_dist(x.tile) > shed_dist(y.tile);
        });
        for (int s : order) insert_best(s);
        std::vector<int> es(st.ents.size());
        std::iota(es.begin(), es.end(), 0);
        // nearanimals=4 (balanced): cows and sheep alternate in placement order (cow, sheep, cow, ...), so the nearest sites are shared
        // like M&M's (cow at ring 0, sheep at ring 1, cows at ring 2) instead of all going to one species.
        std::vector<int> rank(st.ents.size(), 0);
        if (pb.near_animals == 4) {
            int seen[3] = {0, 0, 0};
            for (int e = 0; e < int(st.ents.size()); ++e)
                if (st.ents[e].item == COW || st.ents[e].item == SHEEP) rank[e] = seen[st.ents[e].item - GOOSE]++;
        }
        if (pb.near_animals == 5 || pb.near_animals == 6) {  // M&M's census: one cow at the nearest site, then the sheep, then the other cows
            bool first_cow = true;                              // (6: geese after them, days 0-9: M&M places pasture animals before geese)
            for (int e = 0; e < int(st.ents.size()); ++e)
                if (st.ents[e].item == COW) rank[e] = first_cow ? 0 : 2, first_cow = false;
                else if (st.ents[e].item == SHEEP) rank[e] = 1;
                else if (st.ents[e].item == GOOSE) rank[e] = 3;
        }
        std::stable_sort(es.begin(), es.end(), [&](int a, int b) {
            if (st.ents[a].site_weight != st.ents[b].site_weight) return st.ents[a].site_weight > st.ents[b].site_weight;
            if (pb.near_animals == 5 || pb.near_animals == 6) {
                const auto animal = [&](int e) { const int it = st.ents[e].item; return it == COW || it == SHEEP || (pb.near_animals == 6 && it == GOOSE); };
                return animal(a) && animal(b) && rank[a] < rank[b];
            }
            if (pb.near_animals == 4) {
                const bool pa = st.ents[a].item == COW || st.ents[a].item == SHEEP, pb2 = st.ents[b].item == COW || st.ents[b].item == SHEEP;
                if (pa && pb2) {
                    if (rank[a] != rank[b]) return rank[a] < rank[b];
                    return st.ents[a].item == COW && st.ents[b].item == SHEEP;
                }
                return false;
            }
            if (pb.near_animals >= 2 && pb.near_animals < 4 && (st.ents[a].item == SHEEP) != (st.ents[b].item == SHEEP)) return st.ents[a].item == SHEEP;  // nearanimals
            return false;
        });
        for (int e : es) place_entity(e);
    }
    void insert_best(int s) {
        const Slot slot = best_insertion(s);
        insert_at(slot.route, slot.pos, s);
        route_cost(st.routes[slot.route]);
    }

    // Best site for entity e: an own stop at a free tile or a freeing stop without an entity.
    void place_entity(int e) {
        const Entity en = st.ents[e];
        // nearanimals (dc12, ported from the Codex early-products work): new sheep / cows (and geese at 3) take only the sites nearest the
        // shed that are still free today (hosts or free tiles), so their products reach the shed early.
        const bool near = pb.near_animals && (en.item == SHEEP || en.item == COW || (pb.near_animals == 3 && en.item == GOOSE));
        int nearest = BOARD * 2;
        if (near) {
            for (int tile = 0; tile < BOARD * BOARD; ++tile)
                if (pb.site_release[tile] >= 0 && pb.site_release[tile] <= pb.last_hour && !st.site_used[tile])
                    nearest = std::min(nearest, shed_dist(tile));
            for (int h = 0; h < int(st.stops.size()); ++h)
                if (is_host(h) && st.stops[h].entity < 0 && st.where[h] >= 0)
                    nearest = std::min(nearest, shed_dist(st.stops[h].tile));
        }
        double best = 1e18;
        int best_host = -1, best_tile = -1;
        Slot best_slot;
        for (int h = 0; h < int(st.stops.size()); ++h) {
            if (!is_host(h) || st.stops[h].entity >= 0 || st.where[h] < 0) continue;
            if (near && shed_dist(st.stops[h].tile) != nearest) continue;
            const int r = st.where[h];
            const Route& R = st.routes[r];
            int at = 0;
            while (R.ids[at] != h) ++at;
            const Stop saved = st.stops[h];
            attach(e, h);
            const double d = evaluate(r, R.ids, R.n, at).cost - R.cost + site_cost(en, saved.tile);
            st.stops[h] = saved;
            st.ents[e].stop = -1;
            if (d < best) best = d, best_host = h, best_tile = -1;
        }
        std::vector<std::pair<double, int>> cand;
        for (int tile = 0; tile < BOARD * BOARD; ++tile) {
            if (pb.site_release[tile] < 0 || st.site_used[tile]) continue;
            if (near && shed_dist(tile) != nearest) continue;
            int near = shed_dist(tile);
            for (int s = 0; s < int(st.stops.size()); ++s)
                if (st.where[s] >= 0 && !st.stops[s].shed) near = std::min(near, dist(tile, st.stops[s].tile));
            cand.push_back({site_cost(en, tile) + pb.wage_per_turn * 2 * near, tile});
        }
        std::sort(cand.begin(), cand.end());
        const int tries = std::min<int>(int(cand.size()), pb.site_candidates);
        for (int k = 0; k < tries; ++k) {
            const int tile = cand[k].second;
            const int s = own_stop(e, tile);
            const Slot slot = best_insertion(s);
            const double d = slot.delta + site_cost(en, tile);
            if (d < best) best = d, best_tile = tile, best_host = -1, best_slot = slot;
            st.stops.pop_back();
            st.where.pop_back();
            st.site_used[tile] = 0;
            st.ents[e].stop = -1;
        }
        if (best_host >= 0) {
            attach(e, best_host);
            route_cost(st.routes[st.where[best_host]]);
        } else if (best_tile >= 0) {
            const int s = own_stop(e, best_tile);
            insert_at(best_slot.route, best_slot.pos, s);
            route_cost(st.routes[best_slot.route]);
        }
    }
    void unplace_entity(int e) {
        const int s = st.ents[e].stop;
        if (s < 0) return;
        Stop& stop = st.stops[s];
        if (stop.frees) {
            detach(s);
            if (st.where[s] >= 0) route_cost(st.routes[st.where[s]]);
        } else {
            if (st.where[s] >= 0) {
                const int r = st.where[s];
                erase_stop(s);
                route_cost(st.routes[r]);
            }
            st.site_used[stop.tile] = 0;
            stop.dead = true;
            stop.n = 0;
            stop.entity = -1;
        }
        st.ents[e].stop = -1;
    }

    // ------------------------------------------------------------ local search
    bool relocate_pass() {
        bool improved = false;
        for (int s = 0; s < int(st.stops.size()); ++s) {
            const int r = st.where[s];
            if (r < 0 || s == shed_probe) continue;
            const Route saved = st.routes[r];
            erase_stop(s);
            route_cost(st.routes[r]);
            const double removal = st.routes[r].cost - saved.cost;
            const Slot slot = best_insertion(s, 4);
            if (slot.route >= 0 && removal + slot.delta < -1e-6) {
                insert_at(slot.route, slot.pos, s);
                route_cost(st.routes[slot.route]);
                improved = true;
            } else {
                st.routes[r] = saved;
                st.where[s] = r;
            }
        }
        return improved;
    }

    bool tails_pass() {
        bool improved = false;
        int16_t a[MAX_ROUTE], b[MAX_ROUTE];
        for (int x = 0; x < int(st.routes.size()); ++x)
            for (int y = x + 1; y < int(st.routes.size()); ++y) {
                Route& A = st.routes[x];
                Route& B = st.routes[y];
                PairMemo& memo = tails_memo[x * MAX_WORKERS + y];
                if (memo.a == A.stamp && memo.b == B.stamp) {
                    evaluations += memo.evaluations;
                    continue;
                }
                const long before = evaluations;
                double best = A.cost + B.cost - 1e-6;
                int bi = -1, bj = -1;
                int ta[MAX_ROUTE], tb[MAX_ROUTE];  // stop tiles (shed stops: 44)
                for (int k = 0; k < A.n; ++k) ta[k] = st.stops[A.ids[k]].shed ? 44 : st.stops[A.ids[k]].tile;
                for (int k = 0; k < B.n; ++k) tb[k] = st.stops[B.ids[k]].shed ? 44 : st.stops[B.ids[k]].tile;
                for (int i = 0; i <= A.n; ++i)
                    for (int j = 0; j <= B.n; ++j) {
                        if ((i == A.n && j == B.n) || (i == 0 && j == 0)) continue;
                        // Granular filter: the two new links may not be much longer than the removed ones.
                        const int pa = i ? ta[i - 1] : A.start.tile, pb_ = j ? tb[j - 1] : B.start.tile;
                        const int links = (j < B.n ? dist(pa, tb[j]) : 0) + (i < A.n ? dist(pb_, ta[i]) : 0) -
                                          (i < A.n ? dist(pa, ta[i]) : 0) - (j < B.n ? dist(pb_, tb[j]) : 0);
                        if (links > pb.search_radius) continue;
                        const int na = i + B.n - j, nb = j + A.n - i;
                        if (na >= MAX_ROUTE || nb >= MAX_ROUTE) continue;
                        std::copy_n(A.ids, i, a);
                        std::copy_n(B.ids + j, B.n - j, a + i);
                        std::copy_n(B.ids, j, b);
                        std::copy_n(A.ids + i, A.n - i, b + j);
                        const double c = evaluate(x, a, na, i).cost + evaluate(y, b, nb, j).cost;
                        if (c < best) best = c, bi = i, bj = j;
                    }
                if (bi < 0) {
                    memo = {A.stamp, B.stamp, evaluations - before};
                    continue;
                }
                const int na = bi + B.n - bj, nb = bj + A.n - bi;
                std::copy_n(A.ids, bi, a);
                std::copy_n(B.ids + bj, B.n - bj, a + bi);
                std::copy_n(B.ids, bj, b);
                std::copy_n(A.ids + bi, A.n - bi, b + bj);
                std::copy_n(a, na, A.ids), A.n = na;
                std::copy_n(b, nb, B.ids), B.n = nb;
                for (int k = 0; k < A.n; ++k) st.where[A.ids[k]] = x;
                for (int k = 0; k < B.n; ++k) st.where[B.ids[k]] = y;
                route_cost(A), route_cost(B);
                improved = true;
            }
        return improved;
    }

    bool swap_pass() {
        bool improved = false;
        if (swap_memo.size() < st.stops.size() * MAX_WORKERS) swap_memo.resize(st.stops.size() * MAX_WORKERS);
        for (int s = 0; s < int(st.stops.size()); ++s) {
            if (st.where[s] < 0 || st.stops[s].shed) continue;
            bool gain = false;
            long spent[MAX_WORKERS]{};  // evaluations per partner route (memo)
            for (int t = s + 1; t < int(st.stops.size()); ++t) {
                if (st.where[t] < 0 || st.where[t] == st.where[s] || st.stops[t].shed) continue;
                if (dist(st.stops[s].tile, st.stops[t].tile) > pb.search_radius) continue;
                Route& A = st.routes[st.where[s]];
                Route& B = st.routes[st.where[t]];
                const PairMemo& memo = swap_memo[s * MAX_WORKERS + st.where[t]];
                if (memo.a == A.stamp && memo.b == B.stamp) {
                    evaluations += 2;
                    continue;
                }
                spent[st.where[t]] += 2;
                int i = 0, j = 0;
                while (A.ids[i] != s) ++i;
                while (B.ids[j] != t) ++j;
                A.ids[i] = int16_t(t), B.ids[j] = int16_t(s);
                const double c = evaluate(st.where[s], A.ids, A.n, i).cost + evaluate(st.where[t], B.ids, B.n, j).cost;
                if (c < A.cost + B.cost - 1e-6) {
                    std::swap(st.where[s], st.where[t]);
                    route_cost(A), route_cost(B);
                    improved = gain = true;
                } else {
                    A.ids[i] = int16_t(s), B.ids[j] = int16_t(t);
                }
            }
            if (!gain)
                for (int r = 0; r < int(st.routes.size()); ++r)
                    if (r != st.where[s] && spent[r]) swap_memo[s * MAX_WORKERS + r] = {st.routes[st.where[s]].stamp, st.routes[r].stamp, 0};
        }
        return improved;
    }

    // Group members are interchangeable: move a stop's work to another member tile.
    bool member_pass() {
        bool improved = false;
        // Stops of one kind per tile: crop work (group i), animal service (1000+i), animal output
        // (2000+i), crop output (3000+i); the tile's member group is i (crops) or 1000+i (animals).
        std::vector<int> by_tile(4 * BOARD * BOARD, -1);
        auto base = [](int group) { return group >= 3000 ? group - 3000 : group >= 2000 ? group - 1000 : group; };
        auto key = [](int group, int tile) { return (group / 1000) * BOARD * BOARD + tile; };
        for (int s = 0; s < int(st.stops.size()); ++s)
            if (!st.stops[s].shed && !st.stops[s].dead && st.stops[s].group >= 0) by_tile[key(st.stops[s].group, st.stops[s].tile)] = s;
        for (int s = 0; s < int(st.stops.size()); ++s) {
            if (st.stops[s].shed || st.stops[s].group < 0 || st.where[s] < 0) continue;
            const int g = st.stops[s].group;
            for (int tile = 0; tile < BOARD * BOARD; ++tile) {
                const int a = st.stops[s].tile;
                if (tile == a || pb.member_group[tile] != base(g)) continue;
                const int t = by_tile[key(g, tile)];
                const int rs = st.where[s], rt = t >= 0 ? st.where[t] : -1;
                if (t >= 0 && rt < 0) continue;
                const double before = st.routes[rs].cost + (rt >= 0 && rt != rs ? st.routes[rt].cost : 0);
                st.stops[s].tile = int16_t(tile);
                if (t >= 0) st.stops[t].tile = int16_t(a);
                auto at = [&](int r, int stop) {
                    int k = 0;
                    while (st.routes[r].ids[k] != stop) ++k;
                    return k;
                };
                const int ks = at(rs, s), kt = rt >= 0 ? at(rt, t) : INT_MAX;
                double after = evaluate(rs, st.routes[rs].ids, st.routes[rs].n, rt == rs ? std::min(ks, kt) : ks).cost;
                if (rt >= 0 && rt != rs) after += evaluate(rt, st.routes[rt].ids, st.routes[rt].n, kt).cost;
                const double site = [&] {  // attached entities move with their host tile
                    double d = 0;
                    if (st.stops[s].entity >= 0) d += site_cost(st.ents[st.stops[s].entity], tile) - site_cost(st.ents[st.stops[s].entity], a);
                    if (t >= 0 && st.stops[t].entity >= 0)
                        d += site_cost(st.ents[st.stops[t].entity], a) - site_cost(st.ents[st.stops[t].entity], tile);
                    return d;
                }();
                if (after + site < before - 1e-6) {
                    route_cost(st.routes[rs]);
                    if (rt >= 0 && rt != rs) route_cost(st.routes[rt]);
                    by_tile[key(g, tile)] = s;
                    by_tile[key(g, a)] = t;
                    improved = true;
                } else {
                    st.stops[s].tile = int16_t(a);
                    if (t >= 0) st.stops[t].tile = int16_t(tile);
                }
            }
        }
        return improved;
    }

    bool entity_pass() {
        bool improved = false;
        for (int e = 0; e < int(st.ents.size()); ++e) {
            const double before = total();
            State saved = st;
            unplace_entity(e);
            place_entity(e);
            if (total() < before - 1e-6) improved = true;
            else st = std::move(saved);
        }
        return improved;
    }

    // Shed stops: insert where a deposit or a split trip pays, remove where they do not.
    bool shed_pass() {
        bool improved = false;
        int16_t buf[MAX_ROUTE];
        for (int r = 0; r < int(st.routes.size()); ++r) {
            Route& R = st.routes[r];
            for (int k = 0; k < R.n; ++k) {
                if (!st.stops[R.ids[k]].shed) continue;
                int m = 0;
                for (int j = 0; j < R.n; ++j)
                    if (j != k) buf[m++] = R.ids[j];
                if (evaluate(r, buf, m, k).cost < R.cost - 1e-6) {
                    st.where[R.ids[k]] = -1;
                    std::copy_n(buf, m, R.ids);
                    R.n = m;
                    route_cost(R);
                    improved = true;
                    --k;
                }
            }
            if (R.n + 1 >= MAX_ROUTE) continue;
            double best = R.cost - 1e-6;
            int best_pos = -1;
            for (int p = 1; p <= R.n; ++p) {
                if (st.stops[R.ids[p - 1]].shed || (p < R.n && st.stops[R.ids[p]].shed)) continue;
                std::copy_n(R.ids, p, buf);
                buf[p] = int16_t(shed_probe);
                std::copy_n(R.ids + p, R.n - p, buf + p + 1);
                const double c = evaluate(r, buf, R.n + 1, p).cost;
                if (c < best) best = c, best_pos = p;
            }
            if (best_pos >= 0) {
                insert_at(r, best_pos, new_shed_stop());
                route_cost(R);
                improved = true;
            }
        }
        return improved;
    }

    bool reverse_pass() {
        bool improved = false;
        int16_t buf[MAX_ROUTE];
        for (int r = 0; r < int(st.routes.size()); ++r) {
            Route& R = st.routes[r];
            PairMemo& memo = reverse_memo[r];
            if (memo.a == R.stamp) {
                evaluations += memo.evaluations;
                continue;
            }
            const long before = evaluations;
            bool gain = false;
            for (int i = 0; i + 1 < R.n; ++i)
                for (int j = i + 1; j < R.n; ++j) {
                    std::copy_n(R.ids, R.n, buf);
                    std::reverse(buf + i, buf + j + 1);
                    if (evaluate(r, buf, R.n, i).cost < R.cost - 1e-6) {
                        std::copy_n(buf, R.n, R.ids);
                        route_cost(R);
                        improved = gain = true;
                    }
                }
            if (!gain) memo = {R.stamp, 0, evaluations - before};
        }
        return improved;
    }

    // Spawn tiles (engine rule: least occupied shed tile after that hour's worker actions). The
    // first wave is hired in the farmer's first hour, the second wave one hour later; occupancy
    // comes from the routes of the workers already on the farm. Recomputed after each search round.
    int8_t spawn_at[MAX_WORKERS];
    int tile_after(int r, int hour) {
        struct Track {
            int tile, hour;
            void operator()(int t, UnitAction a) {
                if (t > hour) return;
                if (a.op == OP_NORTH) tile -= BOARD;
                if (a.op == OP_SOUTH) tile += BOARD;
                if (a.op == OP_EAST) tile += 1;
                if (a.op == OP_WEST) tile -= 1;
            }
            void deposit(int, int) const {}
        } track{st.routes[r].start.tile, hour};
        const Route& R = st.routes[r];
        walk(pb, st.stops, R.start, R.ids, R.n, track);
        return track.tile;
    }
    bool sync_spawns() {
        const int workers = int(st.routes.size()), h0 = pb.farmer.hour;
        bool changed = false;
        for (int wave = 0; wave < 2; ++wave) {
            int positions[MAX_WORKERS], count = 0;
            for (int w = 0; w < workers; ++w) {
                const bool present = w == 0 || w - 1 < (wave ? pb.first_wave : 0);
                if (present) positions[count++] = tile_after(w, h0 + wave);
            }
            for (int w = 1; w < workers; ++w) {
                const int index = w - 1;
                if ((index < pb.first_wave) != (wave == 0)) continue;
                const int8_t tile = int8_t(spawn_tile(positions, count));
                positions[count++] = tile;
                spawn_at[w] = tile;
                if (st.routes[w].start.tile == tile) continue;
                st.routes[w].start = start_of(w);
                route_cost(st.routes[w]);
                changed = true;
            }
        }
        return changed;
    }


    void local_search(int rounds = 0) {
        if (!rounds) rounds = pb.search_rounds;
        for (int round = 0; round < rounds; ++round) {
            if (route_prof_on) {  // DC12_ROUTEPROF probe: time per pass
                using clock = std::chrono::steady_clock;
                bool any = false;
                auto timed = [&](int k, auto&& pass) {
                    const auto t = clock::now();
                    any |= pass();
                    route_prof[k] += std::chrono::duration<double, std::milli>(clock::now() - t).count();
                };
                timed(0, [&] { return sync_spawns(); });
                timed(1, [&] { return relocate_pass(); });
                timed(2, [&] { return swap_pass(); });
                timed(3, [&] { return tails_pass(); });
                timed(4, [&] { return reverse_pass(); });
                timed(5, [&] { return member_pass(); });
                timed(6, [&] { return shed_pass(); });
                timed(7, [&] { return entity_pass(); });
                ++route_prof[8];
                if (!any) break;
                continue;
            }
            bool any = sync_spawns();
            any |= relocate_pass();
            any |= swap_pass();
            any |= tails_pass();
            any |= reverse_pass();
            any |= member_pass();
            any |= shed_pass();
            any |= entity_pass();
            if (!any) break;
        }
    }


    // Removes route r's work (a hire's, r > 0): the last route's work moves to r's worker, then the last route is removed.
    void drop_route(int r) {
        Route& last = st.routes.back();
        if (r != int(st.routes.size()) - 1) {
            Route& R = st.routes[r];
            std::swap(R.n, last.n);
            for (int k = 0; k < std::max(R.n, last.n); ++k) std::swap(R.ids[k], last.ids[k]);
            for (int k = 0; k < R.n; ++k) st.where[R.ids[k]] = r;
            route_cost(R);
        }
        drop_last_route();
    }

    // Removes the last route (latest hire) and re-inserts its work.
    void drop_last_route() {
        Route last = st.routes.back();
        st.routes.pop_back();
        for (int k = 0; k < last.n; ++k) st.where[last.ids[k]] = -1;
        for (int k = 0; k < last.n; ++k) {
            const int s = last.ids[k];
            if (st.stops[s].shed) continue;
            if (st.routes.empty()) continue;
            insert_best(s);
        }
    }
};
}

int spawn_tile(const int* positions, int count) {
    int occ[4]{};
    for (int u = 0; u < count; ++u)
        for (int k = 0; k < 4; ++k) occ[k] += positions[u] == SHED_TILES[k];
    int best = 0;
    for (int k = 1; k < 4; ++k)
        if (occ[k] < occ[best]) best = k;
    return SHED_TILES[best];
}

RouteOut route_day(Problem pb) {
    const auto started = std::chrono::steady_clock::now();
    ++route_prof[9];
    Search search(pb);
    int work = 0;
    for (const Stop& s : pb.stops) work += s.n + 2;
    work += 5 * int(pb.entities.size());
    const int capacity = pb.last_hour + 1;
    int hires = std::clamp((work + capacity - 1) / capacity - 1, 0, pb.max_hires);

    struct Candidate { State state; int hires; double cost; int late; };
    std::vector<Candidate> kept;
    auto score = [&](int h) { return wages(h) + search.total(); };
    auto lap = [t = std::chrono::steady_clock::now()](int k) mutable {  // DC12_ROUTEPROF phases: [11] construct, [12] up, [13] down
        const auto now = std::chrono::steady_clock::now();
        route_prof[k] += std::chrono::duration<double, std::milli>(now - t).count();
        t = now;
    };
    search.construct(hires + 1);
    search.local_search();
    lap(11);
    int feasible_at = -1;
    for (;;) {
        const int late = search.late();
        kept.push_back({search.st, hires, score(hires), late});
        if (!late && feasible_at < 0) feasible_at = hires;
        // Past the first complete count, more hires only while they pay (deposits before the market).
        if (hires >= pb.max_hires) break;
        if (feasible_at >= 0 && hires > feasible_at && kept.back().cost > kept[kept.size() - 2].cost - 1e-6) break;
        ++hires;
        search.add_route(hires);
        search.local_search();
    }
    lap(12);
    // Fewer hires below the first complete count, starting from its solution.
    if (feasible_at > 0) {
        for (const auto& c : kept)
            if (c.hires == feasible_at) search.st = c.state;
        for (int h = feasible_at - 1; h >= 0; --h) {
            // dropany: when the last hire's work does not fit, try dissolving a less-loaded hire's route instead
            std::vector<int> order{int(search.st.routes.size()) - 1};
            if (pb.drop_any > 0) {
                std::vector<int> others;
                for (int r = 1; r + 1 < int(search.st.routes.size()); ++r) others.push_back(r);
                std::stable_sort(others.begin(), others.end(), [&](int a, int b) { return search.st.routes[a].n < search.st.routes[b].n; });
                for (int k = 0; k < std::min<int>(pb.drop_any, int(others.size())); ++k) order.push_back(others[k]);
            }
            State before;
            if (order.size() > 1) before = search.st;
            bool fits = false;
            if (pb.down_probe > 0 && order.size() > 1) {  // downprobe: short searches pick the candidate, one full search on it
                int chosen = 0, least = INT_MAX;
                for (size_t k = 0; k < order.size() && !fits; ++k) {
                    if (k) search.st = before;
                    search.drop_route(order[k]);
                    search.local_search(pb.down_probe);
                    const int late = search.late();
                    if (late < least) least = late, chosen = int(k);
                    fits = !late;
                }
                if (!fits) search.st = before, search.drop_route(order[chosen]);
                search.local_search();
                fits = !search.late();
            } else
                for (size_t k = 0; k < order.size() && !fits; ++k) {
                    if (k) search.st = before;
                    search.drop_route(order[k]);
                    search.local_search();
                    route_prof[15] += 1;  // DC12_ROUTEPROF: step-down candidates searched
                    fits = !search.late();
                }
            route_prof[14] += 1;  // DC12_ROUTEPROF: step-down levels tried
            if (!fits) break;
            kept.push_back({search.st, h, score(h), 0});
        }
    }
    lap(13);
    const Candidate* best = &kept[0];
    for (const auto& c : kept)
        if (c.cost < best->cost - 1e-9) best = &c;
    search.st = best->state;
    int hires_used = best->hires;
    while (hires_used > 0 && !search.st.routes.back().n) {  // hires with nothing to do
        search.st.routes.pop_back();
        --hires_used;
    }
    // Tonight's shed room: while the cargo does not fit, deposits before the last market earn
    // (a growing share of) the value of the units that would be destroyed.
    auto overflow = [&] {
        int night = 0, wheat = 0;
        for (const auto& r : search.st.routes) {
            const Eval e = walk(pb, search.st.stops, r.start, r.ids, r.n, NoEmit{});
            night += e.night, wheat += e.night_wheat;
        }
        return night + std::max(0, pb.feed_reserve - wheat) - pb.night_room;
    };
    for (int round = 0; round < (pb.night_rounds ? pb.night_rounds : 3) && overflow() > 0; ++round) {
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int h = 0; h <= pb.last_hour; ++h) pb.gain[p][h] += 0.5 * pb.unit_value[p];
        for (auto& r : search.st.routes) search.route_cost(r);
        search.local_search(3);
    }
    // nighttrim: cargo that still does not fit tonight is destroyed at the night deposit. Output stops whose product cannot be
    // stored are worth nothing today and their product stays on the animal / crop for tomorrow: remove the pure output stops
    // (harvest / collect only, no entity, not survival) that cut the night cargo most until it fits.
    if (pb.night_trim)
        for (int guard = 0; guard < 64 && overflow() > 0; ++guard) {
            int best_r = -1, best_k = -1, best_cut = 0;
            for (int r = 0; r < int(search.st.routes.size()); ++r) {
                const Route& R = search.st.routes[r];
                const Eval base = walk(pb, search.st.stops, R.start, R.ids, R.n, NoEmit{});
                for (int k = 0; k < R.n; ++k) {
                    const Stop& s = search.st.stops[R.ids[k]];
                    if (s.shed || s.entity >= 0 || s.priority == P_SURVIVAL || (s.units <= 0 && s.fertilizer <= 0)) continue;
                    bool pure = true;
                    for (int j = 0; j < s.n && pure; ++j) pure = s.steps[j].op == OP_HARVEST || s.steps[j].op == OP_COLLECT_FERTILIZER;
                    if (!pure) continue;
                    int16_t ids[MAX_ROUTE];
                    int m = 0;
                    for (int j = 0; j < R.n; ++j)
                        if (j != k) ids[m++] = R.ids[j];
                    const Eval e = walk(pb, search.st.stops, R.start, ids, m, NoEmit{});
                    const int cut = base.night - e.night;
                    if (cut > best_cut || (cut == best_cut && cut > 0 && k > best_k)) best_cut = cut, best_r = r, best_k = k;
                }
            }
            if (best_r < 0) break;
            const int stop = search.st.routes[best_r].ids[best_k];
            search.erase_stop(stop);
            search.st.stops[stop].dead = true;  // not re-inserted by later passes
            search.route_cost(search.st.routes[best_r]);
        }
    if (std::getenv("DC12_OVERFLOWLOG") && overflow() > 0) {  // overflow,<left>,room,feed_reserve | per route: night / start cargo / end / stops
        std::string line = "overflow," + std::to_string(overflow()) + "," + std::to_string(pb.night_room) + "," + std::to_string(pb.feed_reserve) + " |";
        for (const auto& r : search.st.routes) {
            const Eval e = walk(pb, search.st.stops, r.start, r.ids, r.n, NoEmit{});
            line += " " + std::to_string(e.night) + "/" + std::to_string(e.end) + "/" + std::to_string(r.n);
        }
        std::fprintf(stderr, "%s\n", line.c_str());
    }
    // The spawn tiles must match the final routes (a search can stop on its round limit).
    for (int round = 0; round < 4 && search.sync_spawns(); ++round) search.local_search(2);

    RouteOut out;
    out.hires = hires_used;
    out.stops = search.st.stops;
    out.entities = search.st.ents;
    for (const auto& r : search.st.routes) {
        const Eval e = walk(pb, out.stops, r.start, r.ids, r.n, NoEmit{});
        std::vector<int> ids(r.ids, r.ids + r.n);
        for (int k = e.served; k < r.n; ++k)
            if (!out.stops[r.ids[k]].shed) out.dropped.push_back(r.ids[k]);
        ids.resize(e.served);
        out.routes.push_back(ids);
        out.starts.push_back(r.start);
        out.moves += e.moves, out.actions += e.actions, out.waits += e.waits;
    }
    for (int e = 0; e < int(out.entities.size()); ++e)
        if (out.entities[e].stop < 0) out.dropped.push_back(-1 - e);
    out.evaluations = search.evaluations;
    route_prof[10] += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    return out;
}

int route_actions(const Problem& pb, const std::vector<Stop>& stops, const Worker& start, const std::vector<int>& route,
                  UnitAction out[HOURS]) {
    for (int h = 0; h < HOURS; ++h) out[h] = UnitAction{};
    struct Emit {
        UnitAction* out;
        int last;
        void operator()(int t, UnitAction a) const {
            if (t >= 0 && t <= last) out[t] = a;
        }
        void deposit(int, int) const {}
    } emit{out, pb.last_hour};
    std::vector<int16_t> ids(route.begin(), route.end());
    return walk(pb, stops, start, ids.data(), int(ids.size()), emit).end;
}
}
