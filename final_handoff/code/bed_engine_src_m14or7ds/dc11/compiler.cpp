#include "dc11/compiler.hpp"
#include "dc11_local/sellmodel.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <numeric>
#include <cstdio>
#include <cstdlib>

namespace dc11 {

const char* status_name(CompileStatus s) {
    switch (s) {
        case CompileStatus::Ok: return "ok";
        case CompileStatus::InvalidIntent: return "invalid_intent";
        case CompileStatus::NoSchedule: return "no_schedule";
        case CompileStatus::Unfunded: return "unfunded";
    }
    return "?";
}

double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

namespace {

// ---------------------------------------------------------------- binding
struct Bound {
    Problem pb;
    int land_hour = -1;
    int reserve_target[N_ANIMALS]{};  // unplaced animals after tonight (intent reserve)
    int sale_slots = 0;                // saleslots: first-hour order slots the seller needs (products with dawn stock)
    bool wheat_slot = false;           // saleslots=2: the dawn wheat sale (wheatcash) needs a first-hour slot too
    int sale_want = 0;                 // saleslots=3: thin products the seller's dawn decision sells (slots priced against the crew)
    int output[N_PRODUCTS]{};      // expected output of today's harvests and collections
    int new_animals[N_ANIMALS]{};
    int entities = 0;
    int dawn_wheat_sale = 0;  // wheatcash: dawn shed wheat sold at hour 0 (not in the routes' dawn stock)
    bool land_first = false;  // cashsell: the land is ordered before the hour's other purchases (it gates the new quadrant's work)
};

int water_gain(const CropGroup& g, int option) {
    const auto& c = CROPS[g.crop];
    if (!has_water(option) || g.age < (c.max_yield_day + 1) / 2 || g.age > c.max_yield_day) return 0;
    const bool fertilized = g.fert_days >= 1 || has_fertilize(option);
    return std::min<int>(c.max_yield - g.yield, fertilized ? 2 : 1);
}

bool production_tonight(const CropGroup& g, int day) {
    const auto& c = CROPS[g.crop];
    if (day >= LAST_DAY) return false;
    const int since = g.age + 1 - c.first_yield_day;
    return since >= 0 && since % c.interval == 0 && since / c.interval < c.max_yield;
}

// Members nearest the shed first (initial binding; the router may swap members).
template <class Group>
std::vector<int> members_by_distance(const Group& g) {
    std::vector<int> cells(g.cells.begin(), g.cells.begin() + g.size);
    std::stable_sort(cells.begin(), cells.end(), [](int a, int b) { return shed_dist(a) < shed_dist(b); });
    return cells;
}

// Land bought at `hour`: its tiles are sites from the next hour.
void set_land_hour(Bound& b, const agent::AgentObservation& dawn, int hour) {
    b.land_hour = hour;
    for (int cell = 0; cell < BOARD * BOARD; ++cell)
        if (b.pb.tiles[cell].kind == T_LOCKED && quadrant_of(cell % BOARD, cell / BOARD, BOARD) == dawn.self().n_quadrants)
            b.pb.site_release[cell] = int8_t(hour + 1);
}

Bound bind(const agent::AgentObservation& dawn, const Schema& s, const DayIntent& in, int level, const Options& options) {
    Bound b;
    Problem& pb = b.pb;
    const int day = s.day;
    pb.day = day;
    pb.last_hour = last_hour(day);
    pb.wage_per_turn = options.wage_per_turn;
    const bool q4 = dawn.self().n_quadrants >= 4;
    pb.search_radius = q4 && options.q4_radius ? options.q4_radius : options.search_radius;
    pb.search_rounds = q4 && options.q4_rounds ? options.q4_rounds : options.search_rounds;
    pb.night_rounds = options.night_rounds;
    pb.site_candidates = options.site_candidates;
    pb.optional_late = options.optional_late;
    pb.near_animals = day <= 5 || options.near_animals == 3 || (options.near_animals == 6 && day <= 9) ? options.near_animals : 0;  // nearanimals: days 0-5 (3: every day, geese too)
    pb.return_cost = options.return_cost;
    pb.final_return = options.final_return_always || day == LAST_DAY;
    pb.place_deposits = options.regime_on(day) && day < LAST_DAY;
    pb.animal_morning = options.regime_on(day) ? options.regime_morning : 0;
    pb.reserve_animal_sites = day < 16;
    if (options.sale_slots == 1)  // the engine takes 10 orders a turn, one per hire: the dawn sales need their own slots
        for (int p = 0; p < N_PRODUCTS; ++p) b.sale_slots += p != FERTILIZER && dawn.own.shed[p] > 0;
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        pb.tiles[cell] = tile_at(dawn, cell);
        const Tile& t = pb.tiles[cell];
        const bool open = t.kind == T_EMPTY || t.kind == T_WEED || ((t.kind == T_COOP || t.kind == T_PASTURE) && !t.has_animal);
        pb.site_release[cell] = open ? 0 : -1;
    }
    for (int i = 0; i < N_CARRY; ++i) pb.dawn_stock[i] = dawn.own.shed[CARRY_ITEM[i]];
    for (int c = 0; c < N_CROPS; ++c) pb.dawn_seeds[c] = dawn.own.seeds[c];
    const bool survival = level >= SurvivalOnly, no_new = level >= NoNewEntities;
    const bool land = in.buy_land && level < NoLand && !no_new;
    if (land) set_land_hour(b, dawn, 0);
    pb.drop_any = land ? 0 : options.drop_any;
    if (dawn.own.shed[WHEAT] == 0) pb.wheat_buy_cost = options.feed_cost * dawn.market.prices[WHEAT];  // feedcost: all feed wheat is bought
    pb.down_probe = options.down_probe;
    pb.night_trim = options.night_trim;  // land days: a smaller crew there broke the land's funding (day 10 fallbacks)
    auto price = [&](int p) { return double(dawn.market.prices[p]); };
    int feed_budget = INT_MAX;
    if (survival) {
        const double wheat = std::max(1, market_price(WHEAT, dawn.market.inventory[WHEAT] - 1));
        double income = 0;
        for (int i = 0; i < s.n_crops; ++i) {
            const CropGroup& g = s.crops[i];
            if (g.fresh) continue;
            const int harvested = CROPS[g.crop].ongoing ? in.harvest[i] : [&] {
                int n = 0;
                for (int o = 0; o < OPTIONS; ++o) n += has_harvest(o) ? in.options[i][o] : 0;
                return n;
            }();
            income += 0.8 * harvested * g.yield * price(g.crop);
        }
        for (int i = 0; i < s.n_animals; ++i)
            if (!s.animals[i].fresh) income += 0.8 * in.collect[i] * s.animals[i].held * price(ANIMALS[s.animals[i].species].product);
        feed_budget = int(dawn.own.shed[WHEAT]) + int((std::max(0.0, dawn.self().money) + income) / (wheat * 1.1));
    }
    auto add_stop = [&](Stop st) { pb.stops.push_back(st); };
    // ---- one-shot crops
    for (int i = 0; i < s.n_crops; ++i) {
        const CropGroup& g = s.crops[i];
        if (g.fresh) continue;
        const auto& crop = CROPS[g.crop];
        const auto cells = members_by_distance(g);
        const Tile& t0 = tile_at(dawn, cells[0]);
        const int decay_from = t0.max_lifespan_step >= 0 ? std::max(0, t0.max_lifespan_step - day * HOURS) : -1;
        if (!crop.ongoing) {
            std::vector<int> order;
            for (int o = 0; o < OPTIONS; ++o)
                for (int n = 0; n < in.options[i][o]; ++n) order.push_back(o);
            std::stable_sort(order.begin(), order.end(), [](int a, int c) {
                auto work = [](int o) { return o == CLEAR ? 1 : std::popcount(unsigned(o)); };
                return work(a) > work(c);
            });
            const bool weed_today = turns_weed_today(g);
            for (size_t m = 0; m < cells.size(); ++m) {
                int o = order[m];
                if (survival) o = (has_harvest(o) ? 1 : 0) | ((has_water(o) && g.dry >= 1) ? 4 : 0);
                Stop st;
                st.tile = int16_t(cells[m]);
                st.group = weed_today ? -1 : int16_t(i);
                if (!weed_today) pb.member_group[cells[m]] = int16_t(i);
                if (o == CLEAR) {
                    st.steps[st.n++] = {OP_DIG, 0};
                    st.frees = true;
                    st.priority = P_EXTRA;
                } else {
                    if (has_fertilize(o)) st.steps[st.n++] = {OP_FERTILIZE, 0};
                    if (has_water(o)) st.steps[st.n++] = {OP_WATER, 0};
                    if (has_harvest(o)) {
                        st.harvest_at = int8_t(st.n);
                        st.steps[st.n++] = {OP_HARVEST, 0};
                        st.product = int8_t(g.crop);
                        st.units = int8_t(g.yield + water_gain(g, o));
                        st.decay_from = int8_t(decay_from);
                        st.unit_value = float(price(g.crop));
                        st.frees = true;
                        st.priority = P_OUTPUT;
                        st.value = float(st.units * price(g.crop));
                        if (g.age <= crop.max_yield_day || decay_from < 0) b.output[g.crop] += st.units;
                        else b.output[g.crop] += std::max(0, int(st.units) - 2);
                    } else if (has_water(o)) {
                        st.priority = g.dry >= 1 ? P_SURVIVAL : P_OUTPUT;
                        st.value = float(crop.seed + g.yield * price(g.crop));
                    }
                }
                if (!st.n) {
                    if (weed_today && !no_new) pb.site_release[cells[m]] = 0;  // dug and reused
                    continue;
                }
                add_stop(st);
            }
            continue;
        }
        // ---- ongoing crops: retained nearest, then cleared, then abandoned; harvest the nearest
        const int r = in.retain[i], c = in.clear[i];
        const bool terminal = day == LAST_DAY;
        for (size_t m = 0; m < cells.size(); ++m) {
            Stop st;
            st.tile = int16_t(cells[m]);
            st.group = int16_t(i);
            pb.member_group[cells[m]] = int16_t(i);
            const bool retained = !terminal && int(m) < r;
            const bool cleared = !terminal && !survival && int(m) >= r && int(m) < r + c;
            const bool harvested = int(m) < in.harvest[i];
            const bool fertilized = int(m) < in.fertilize[i] && !survival;
            if (harvested) {
                st.harvest_at = int8_t(st.n);
                st.steps[st.n++] = {OP_HARVEST, 0};
                st.product = int8_t(g.crop);
                st.units = g.yield;
                st.decay_from = int8_t(g.decaying ? decay_from : -1);
                st.unit_value = float(price(g.crop));
                st.priority = P_OUTPUT;
                st.value = float(g.yield * price(g.crop));
                b.output[g.crop] += g.decaying ? std::max(0, g.yield - 2) : g.yield;
            }
            if (fertilized) st.steps[st.n++] = {OP_FERTILIZE, 0};
            if (retained) {
                const int held = harvested ? 0 : g.yield;
                const bool active = g.fert_days >= 1 || fertilized;
                const bool doubled = production_tonight(g, day) && active && held + 2 <= crop.max_yield;
                if (g.dry >= 1 || doubled) st.steps[st.n++] = {OP_WATER, 0};
                if (g.dry >= 1) st.priority = P_SURVIVAL, st.value = float(crop.seed + 2 * price(g.crop));
            }
            if (cleared) {
                st.steps[st.n++] = {OP_DIG, 0};
                st.frees = true;
            }
            if (st.n) add_stop(st);
        }
    }

    // ---- animals: a service stop (feed, care) and an output stop (product, fertilizer) per
    // animal, so output can be sold before the feed it finances is bought.
    // collectmin (the teammate's econ package): fertilizer collections are skipped while fertilizer sells below this price,
    // unless the shed plus the collections so far are short of today's fertilize steps.
    int fertilizer_need = 0;
    for (int i = 0; i < s.n_crops; ++i) {
        fertilizer_need += in.fertilize[i];
        if (!CROPS[s.crops[i].crop].ongoing)
            for (int o = 0; o < OPTIONS; ++o)
                if (has_fertilize(o)) fertilizer_need += in.options[i][o];
    }
    int fertilizer_collected_for_need = 0;
    int feeds = 0;
    for (int i = 0; i < s.n_animals; ++i) {
        const AnimalGroup& g = s.animals[i];
        if (g.fresh) continue;
        const auto cells = members_by_distance(g);
        const int product = ANIMALS[g.species].product;
        // keepfed: an animal unfed yesterday escapes tonight unless fed. Feed it whenever its remaining production is worth more
        // than keeping it fed every other day to the end (the intent sometimes leaves such groups unfed mid-game).
        int feed_n = in.feed[i];
        if (std::getenv("DC12_KEEPFEDLOG") && g.unfed >= 1)
            std::fprintf(stderr, "keepfed day %d level %d group %d species %d size %d feed %d keep %d budget %d price %.0f wheat %.0f held %d\n", day, level, i, g.species, int(cells.size()),
                         in.feed[i], int(options.keep_fed), feed_budget, price(product), price(WHEAT), int(g.held));
        if (options.keep_fed && g.unfed >= 1 && feed_n < int(cells.size())) {
            const int left = LAST_DAY - day, units = left / ANIMALS[g.species].interval;
            if (0.8 * units * price(product) > (left + 1) / 2 * price(WHEAT)) feed_n = int(cells.size());
        }
        // At the held cap with production tonight: uncollected product would be lost.
        const int since = g.age + 1 - ANIMALS[g.species].first_yield_day;
        const bool capped = options.cap_collect && g.held >= ANIMALS[g.species].max_held && since >= 0 &&
                            since % ANIMALS[g.species].interval == 0;
        for (size_t m = 0; m < cells.size(); ++m) {
            pb.member_group[cells[m]] = int16_t(1000 + i);
            Stop service, output;
            service.tile = output.tile = int16_t(cells[m]);
            service.animal = output.animal = true;
            service.group = int16_t(1000 + i);
            output.group = int16_t(2000 + i);
            if (int(m) < feed_n && feeds < feed_budget) {
                service.steps[service.n++] = {OP_FEED, 0};
                ++feeds;
                service.priority = g.unfed >= 1 ? P_SURVIVAL : P_OUTPUT;
                service.value = float(g.unfed >= 1 ? ANIMALS[g.species].cost : 30);
            }
            if (int(m) < in.care[i] && !survival) {
                service.steps[service.n++] = {OP_CARE, 0};
                if (service.n == 1) service.value = 20;
            }
            const bool cheap = options.product_min > 0 && day < LAST_DAY - 1 && price(product) < options.product_min * MARKET[product].base;
            if ((int(m) < in.collect[i] || capped) && !cheap) {
                output.harvest_at = int8_t(output.n);
                output.steps[output.n++] = {OP_HARVEST, 0};
                output.product = int8_t(product);
                output.units = g.held;
                output.unit_value = float(price(product));
                output.value = float(g.held * price(product));
                b.output[product] += g.held;
                if (options.defer_collect && !capped && day < LAST_DAY - 1 && g.held + 2 <= ANIMALS[g.species].max_held) {
                    output.priority = P_EXTRA;
                    output.value = float(g.held * price(product) * (1 - options.market.hold_discount));
                }
            }
            if ((service.n || output.n || !survival) && level < NoCollection &&
                tile_at(dawn, cells[m]).fertilizer_available &&
                (options.collect_min <= 0 || price(FERTILIZER) >= options.collect_min ||
                 dawn.own.shed[FERTILIZER] + fertilizer_collected_for_need < fertilizer_need)) {
                // splitfert=D (dc12, days 3..D-1): a milk / wool / egg harvest stop carries no fertilizer collection; the fertilizer is
                // its own optional stop on the tile, so the harvest can ride a short trip and the fertilizer a later visit (our units
                // waited ~1 h in pockets for the collection; M&M collects 79-81% of that fertilizer later, by another worker)
                const bool split = options.split_fert > 1 && day >= 3 && day < options.split_fert && output.n > 0 &&
                                   (product == MILK || product == WOOL || product == EGG);
                Stop fert;
                fert.tile = output.tile, fert.animal = true, fert.group = -1, fert.priority = P_EXTRA;
                Stop& to = split ? fert : output;
                to.steps[to.n++] = {OP_COLLECT_FERTILIZER, 0};
                to.fertilizer = 1;
                to.value += float(0.9 * price(FERTILIZER));
                ++b.output[FERTILIZER];
                ++fertilizer_collected_for_need;
                if (!split && output.n == 1) output.priority = P_EXTRA;  // a fertilizer-only output stop
                if (split) add_stop(fert);
            }
            if (service.n) add_stop(service);
            if (output.n) add_stop(output);
        }
    }

    // ---- new crops and animals
    if (!no_new) {
        for (int crop = 0; crop < N_CROPS; ++crop) {
            const int gi = s.new_crop_group[crop];
            auto add = [&](uint8_t extra) {
                Entity e;
                e.item = uint8_t(crop);
                e.extra = extra;
                e.site_weight = float(options.site_scale) * (CROPS[crop].ongoing ? 1.0f : crop == MELON ? 0.8f : 0.4f);
                e.value = float(3 * CROPS[crop].seed);
                pb.entities.push_back(e);
            };
            if (CROPS[crop].ongoing)
                for (int n = 0; n < in.new_crop[crop]; ++n) add(2);
            else
                for (int o = 0; o < OPTIONS; ++o)
                    for (int n = 0; n < in.options[gi][o]; ++n) add(uint8_t(2 | (has_fertilize(o) ? 1 : 0)));
        }
        for (int a = 0; a < N_ANIMALS; ++a) {
            const int gi = s.new_animal_group[a];
            for (int n = 0; n < in.new_animal[a]; ++n) {
                Entity e;
                e.item = uint8_t(GOOSE + a);
                e.extra = uint8_t((n < in.feed[gi] ? 4 : 0) | (n < in.care[gi] ? 8 : 0));
                e.site_weight = float(options.site_scale) * 2.0f;
                e.value = float(ANIMALS[a].cost);
                pb.entities.push_back(e);
            }
            b.new_animals[a] = in.new_animal[a];
            b.reserve_target[a] = in.reserve[a];
        }
    }
    b.entities = int(pb.entities.size());
    for (const auto& e : pb.entities)
        if (e.item < N_CROPS) ++pb.plants[e.item];
    // Tonight: the shed keeps the unplaced animals and tomorrow's first feed (1 wheat per animal).
    for (int p = 0; p < N_PRODUCTS; ++p) pb.unit_value[p] = price(p);
    if (day < LAST_DAY) {
        int animals = 0;
        for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += pb.tiles[cell].has_animal;
        for (int a = 0; a < N_ANIMALS; ++a) animals += b.new_animals[a];
        pb.feed_reserve = animals;
        pb.night_room = 95;
        for (int a = 0; a < N_ANIMALS; ++a) pb.night_room -= b.reserve_target[a];
    }
    return b;
}

// ---------------------------------------------------------------- realization
struct Realized {
    bool ok = false;
    std::string why;
    Action actions[HOURS]{};
    int receipts[HOURS][N_PRODUCTS]{};
    int night_carried = 0, night_items[N_PRODUCTS]{};
    int spawn[MAX_WORKERS];        // actual spawn tile of hire k (index = unit)
    int first_wave = -1;           // hires that fit the first hour's orders (-1: all planned did)
    int first_orders = 0;          // the first hour's land / seed / item orders
    std::vector<int> unfinished;   // routes whose actions run past the day
    int placed[N_ANIMALS]{};
};

// Plays the routes in the engine (unlimited cash and shed room, no night update). Each worker's actions are
// generated when it appears, from its actual tile; new hires take the unassigned route whose
// first stop is nearest. Pass 0 finds the purchases (bought the hour before first use); pass 1
// replays with them and checks every unit action.
Realized realize(const agent::AgentObservation& dawn, const Bound& b, const RouteOut& out) {
    Realized z;
    std::fill_n(z.spawn, MAX_WORKERS, -1);
    const Problem& pb = b.pb;
    const int hours = pb.last_hour + 1, me = dawn.player, first = pb.farmer.hour;
    const int new_hires = out.hires;
    const int wave0 = std::min(new_hires, pb.first_wave);
    const int n_routes = int(out.routes.size());
    int seeds[HOURS][N_CROPS]{}, items[HOURS][N_ITEMS]{};
    Action pass;
    pass.finalize();
    for (int round = 0; round < 2; ++round) {
        Config config;
        Sim day = sim_from_observation(dawn, config);
        day.st.farms[me].money = 1e12;
        day.st.farms[me].shed[WHEAT] -= Count(b.dawn_wheat_sale);  // wheatcash: sold at hour 0
        day.st.farms[me].shed_total -= b.dawn_wheat_sale;
        day.cfg.turns_per_day = 1 << 20;  // no night update: tonight's cargo stays visible
        day.cfg.shed_capacity = 1 << 20;  // no sales here: the executor sells to make room (funding checks it)
        static_assert(MAX_WORKERS <= 16);
        UnitAction queue[MAX_WORKERS][HOURS];
        int route_of[MAX_WORKERS];
        std::fill_n(route_of, MAX_WORKERS, -1);
        bool assigned[MAX_WORKERS]{};
        int ends[MAX_WORKERS]{};
        int known = 0, hired = 0;
        int received[N_PRODUCTS]{};
        for (int h = first; h < hours; ++h) {
            Farm& f = day.st.farms[me];
            // Workers that act from this hour: bind a route and generate its actions.
            for (int u = known; u < f.n_units; ++u) {
                const int tile = f.pos_y[u] * BOARD + f.pos_x[u];
                int best = -1, score = INT_MAX;
                for (int r = 0; r < n_routes; ++r) {
                    if (assigned[r]) continue;
                    if (u == 0 && r != 0) continue;  // the farmer keeps route 0
                    const int at = out.routes[r].empty() ? tile : out.stops[out.routes[r][0]].shed ? 44 : out.stops[out.routes[r][0]].tile;
                    const int s = 1000 * std::abs(out.starts[r].hour - h) + 100 * dist(out.starts[r].tile, tile) + dist(tile, at);
                    if (s < score) score = s, best = r;
                }
                if (best < 0) continue;
                assigned[best] = true;
                route_of[u] = best;
                Worker w = out.starts[best];
                w.tile = int8_t(tile);
                w.hour = int8_t(h);
                ends[best] = route_actions(pb, out.stops, w, out.routes[best], queue[u]);
                if (u > 0) z.spawn[u] = tile;
            }
            known = f.n_units;
            Action a;
            a.n_units = f.n_units;
            for (int u = 0; u < a.n_units; ++u) a.units[u] = route_of[u] >= 0 ? queue[u][h] : UnitAction{};
            if (round == 0) {
                int plant[N_CROPS]{}, pickup[N_ITEMS]{};
                for (int u = 0; u < a.n_units; ++u) {
                    if (a.units[u].op == OP_PLANT && a.units[u].arg < N_CROPS) ++plant[a.units[u].arg];
                    if (a.units[u].op == OP_PICKUP && a.units[u].arg < N_ITEMS) pickup[a.units[u].arg] += a.units[u].n;
                }
                for (int c = 0; c < N_CROPS; ++c)
                    if (plant[c] > f.seeds[c]) {
                        if (h == first) return z.why = "first-hour plant without seed", z;
                        seeds[h - 1][c] += plant[c] - f.seeds[c];
                        f.seeds[c] = Count(plant[c]);
                    }
                for (int i : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)})
                    if (pickup[i] > f.shed[i]) {
                        if (h == first) return z.why = "first-hour pickup without stock", z;
                        const int d = pickup[i] - f.shed[i];
                        items[h - 1][i] += d;
                        f.shed[i] = Count(pickup[i]);
                        f.shed_total += d;
                    }
            }
            // Orders in the order cash should go: hires (first wave now, the rest next hour), feed and
            // fertilizer, animals, seeds, land.
            int due = h == first ? wave0 : h >= first + 1 ? new_hires : 0;
            if (pb.cash_wave < new_hires && h < pb.cash_hire_hour) due = std::min(due, pb.cash_wave);  // latehire: the rest at cash_hire_hour
            for (; hired < due; ++hired) a.orders[a.n_orders++] = {M_HIRE, 0, 1};
            if (b.land_first && b.land_hour == h) a.orders[a.n_orders++] = {M_BUY_LAND, 0, 1};
            if (round == 1) {
                for (int i : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)})
                    if (items[h][i]) a.orders[a.n_orders++] = {uint8_t(is_animal(uint8_t(i)) ? M_BUY_ANIMAL : M_BUY_PRODUCT), uint8_t(i), items[h][i]};
                for (int c = 0; c < N_CROPS; ++c)
                    if (seeds[h][c]) a.orders[a.n_orders++] = {M_BUY_SEED, uint8_t(c), seeds[h][c]};
            }
            if (!b.land_first && b.land_hour == h) a.orders[a.n_orders++] = {M_BUY_LAND, 0, 1};
            if (round == 1 && a.n_orders > 10) return z.why = "more than 10 orders at hour " + std::to_string(h), z;
            a.finalize();
            if (round == 1) {
                z.actions[h] = a;
                for (int u = 0; u < a.n_units; ++u) {
                    const auto& x = a.units[u];
                    if (x.op == OP_DROP)
                        for (int p = 0; p < N_PRODUCTS; ++p) received[p] += f.inv[u][p];
                    if (x.op == OP_PLACE && x.arg < N_PRODUCTS) received[x.arg] += std::min<int>(x.n, f.inv[u][x.arg]);
                }
                std::copy_n(received, N_PRODUCTS, z.receipts[h]);
                const auto diag = (me == 0 ? day.diagnose_joint_actions(a, pass) : day.diagnose_joint_actions(pass, a)).players[me];
                if (diag.successful_unit_actions != diag.requested_unit_actions) {
                    z.why = "unit action fails at hour " + std::to_string(h);
                    for (int u = 0; u < a.n_units; ++u) {  // name the first failing unit (units act in list order)
                        Action one;
                        one.n_units = a.n_units;
                        for (int v = 0; v <= u; ++v) one.units[v] = a.units[v];
                        one.finalize();
                        const auto d = (me == 0 ? day.diagnose_joint_actions(one, pass) : day.diagnose_joint_actions(pass, one)).players[me];
                        if (d.successful_unit_actions == d.requested_unit_actions) continue;
                        const Tile& t = f.tiles[f.pos_y[u]][f.pos_x[u]];
                        z.why += " unit " + std::to_string(u) + " op " + std::to_string(a.units[u].op) + " arg " +
                                 std::to_string(a.units[u].arg) + " at " + std::to_string(f.pos_y[u] * BOARD + f.pos_x[u]) + " kind " +
                                 std::to_string(t.kind) + " what " + std::to_string(t.what) + " yield " + std::to_string(t.yield_units) +
                                 " inv w" + std::to_string(f.inv[u][WHEAT]) + " f" + std::to_string(f.inv[u][FERTILIZER]);
                        break;
                    }
                    return z;
                }
            }
            me == 0 ? day.step(a, pass) : day.step(pass, a);
        }
        const Farm& end = day.st.farms[me];
        if (round == 0) {
            // First-hour orders: purchases (and, saleslots, the dawn sales) go first, hires fill the free slots.
            int orders = (b.land_hour == first) + b.sale_slots + (b.wheat_slot && b.dawn_wheat_sale > 0);
            for (int c = 0; c < N_CROPS; ++c) orders += seeds[first][c] > 0;
            for (int i = 0; i < N_ITEMS; ++i) orders += items[first][i] > 0;
            z.first_orders = orders - b.sale_slots - (b.wheat_slot && b.dawn_wheat_sale > 0);
            if (orders + wave0 > 10) {
                z.first_wave = std::max(0, 10 - orders);
                z.why = "first-hour orders";
                return z;
            }
            // Animals for tonight's shed: the intent's unplaced count, bought at the last market.
            for (int a = 0; a < N_ANIMALS; ++a) {
                int held = end.shed[GOOSE + a];
                for (int u = 0; u < end.n_units; ++u) held += end.inv[u][GOOSE + a];
                items[hours - 1][GOOSE + a] += std::max(0, b.reserve_target[a] - held);
            }
            continue;
        }
        for (int h = hours; h < HOURS; ++h) std::copy_n(received, N_PRODUCTS, z.receipts[h]);
        for (int u = 0; u < end.n_units; ++u)
            for (int i = 0; i < N_ITEMS; ++i) {
                z.night_carried += end.inv[u][i];
                if (i < N_PRODUCTS) z.night_items[i] += end.inv[u][i];
            }
        for (int r = 0; r < n_routes; ++r)
            if (!assigned[r] || ends[r] > hours) {
                z.unfinished.push_back(r);
                z.why += " r" + std::to_string(r) + (assigned[r] ? " end" + std::to_string(ends[r]) : " unassigned");
            }
    }
    z.ok = true;
    return z;
}

// ---------------------------------------------------------------- funding
struct Funding {
    bool ok = false;
    std::string why;
    int fail_hour = -1;
    int fail_op = -1, fail_item = -1;  // failed order (op, item); -1: unit action or reserve
    bool reserve = false;              // next-dawn reserve failed
    double value = 0;                  // our money + held stock - rival_weight x the opponent's money at day end
    double skipped_seed = 0;           // achieve: seed value of the plantings the executor skipped (seeds short)
    int hires_filled = -1;             // hirecheck: hires filled by the end of the failing hour's wave
};

double planned_cost(const agent::AgentObservation& obs, const Action& planned) {
    double cost = 0;
    int hires = obs.self().hires_today;
    for (int k = 0; k < planned.n_orders; ++k) {
        const auto& o = planned.orders[k];
        if (o.op == M_HIRE) cost += fib(hires++);
        if (o.op == M_BUY_LAND) cost += LAND_PRICES[std::min(2, obs.self().n_quadrants - 1)];
        if (o.op == M_BUY_SEED) cost += double(CROPS[o.item].seed) * o.n;
        if (o.op == M_BUY_ANIMAL) cost += double(ANIMALS[o.item - GOOSE].cost) * o.n;
        if (o.op == M_BUY_PRODUCT)
            for (int k2 = 0; k2 < o.n; ++k2) cost += market_price(o.item, obs.market.inventory[o.item] - 1 - k2);
    }
    return cost;
}

// feedlate: an order at the last market hour buys tomorrow's first feed (1 wheat per animal beyond the wheat carried tonight). The
// executor's reserve then no longer keeps that wheat in the shed during the day.
void add_late_feed(const agent::AgentObservation& dawn, Plan& plan) {
    int animals = 0;
    for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(dawn, cell).has_animal;
    for (int a = 0; a < N_ANIMALS; ++a) animals += plan.new_animals[a];
    const int need = animals - plan.night_items[WHEAT];
    if (need <= 0) return;
    Action& last = plan.actions[plan.hours - 1];
    for (int k = 0; k < last.n_orders; ++k)
        if (last.orders[k].op == M_BUY_PRODUCT && last.orders[k].item == WHEAT) {
            last.orders[k].n += need;
            return;
        }
    if (last.n_orders < 8) last.orders[last.n_orders++] = {M_BUY_PRODUCT, WHEAT, need};
}

// Plays the plan with the executor against an opponent that sells the given flow; every planned
// order must be submitted and filled, and the farm must end the day able to feed tomorrow.
Funding funded(const agent::AgentObservation& dawn, const History& history, const Plan& plan, const Options& options,
               const double rival[HOURS][N_PRODUCTS]) {
    Funding r;
    Config config;
    Sim sim = sim_from_observation(dawn, config);
    Executor executor;
    struct Quiet { Quiet() { g_sell_quiet = true; } ~Quiet() { g_sell_quiet = false; } } quiet;  // DC11_AUDIT: live hours only
    Options simulated = options;
    simulated.market.intraday = nullptr;  // no opponent sales of today exist in this simulation
    simulated.market.carry_target = nullptr;
    simulated.market.next_morning = nullptr;  // local addition (nextm)
    simulated.market.scenarios = 0;       // the funding check uses the deterministic seller (scen: live executor only)
    simulated.sell_model = nullptr, simulated.sell_tracker = nullptr;  // sellmodel: live executor only (a simulation must not feed the game's tracker)
    executor.start(dawn, history, plan, simulated);
    const int me = dawn.player;
    for (int h = 0; h < plan.hours; ++h) {
        const auto obs = agent::runtime::make_observation(sim, me);
        Action mine;
        executor.act(obs, history, mine);
        Action other;
        Farm& opponent = sim.st.farms[1 - me];
        other.n_units = opponent.n_units;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            const int n = int(std::lround(std::max(0.0, rival[h][p])));
            if (!n || other.n_orders >= 10) continue;
            opponent.shed[p] = Count(n);
            other.orders[other.n_orders++] = {M_SELL, uint8_t(p), n};
        }
        other.finalize();
        const auto diag = (me == 0 ? sim.diagnose_joint_actions(mine, other) : sim.diagnose_joint_actions(other, mine)).players[me];
        // Every submitted order unit filled: the accepted orders are the submitted ones. Otherwise the engine's sanitizer
        // (its per-unit state hashes cost ~14% of the compile when run every hour).
        Action accepted;
        if (diag.successful_order_units == diag.requested_order_units && mine.n_orders <= sim.cfg.max_orders) {
            accepted.n_orders = mine.n_orders;
            for (int j = 0; j < mine.n_orders; ++j) {
                const auto& o = mine.orders[j];
                if (o.op == M_HIRE || o.op == M_BUY_LAND) accepted.orders[j] = {o.op, 0, 1};
                else if (o.op != M_NONE && o.n > 0) accepted.orders[j] = o;
            }
        } else {
            accepted = me == 0 ? sim.sanitize_joint_actions(mine, other)[0] : sim.sanitize_joint_actions(other, mine)[1];
        }
        if (std::getenv("DC11_FUNDDEBUG")) {
            std::fprintf(stderr, "fund h%d money %.0f shed", h, obs.self().money);
            for (int i = 0; i < N_ITEMS; ++i) std::fprintf(stderr, " %d", obs.own.shed[i]);
            std::fprintf(stderr, " | orders");
            for (int k = 0; k < mine.n_orders; ++k)
                std::fprintf(stderr, " %d/%d/%d->%d", mine.orders[k].op, mine.orders[k].item, mine.orders[k].n, accepted.orders[k].n);
            std::fprintf(stderr, " | units");
            for (int u = 0; u < mine.n_units; ++u) std::fprintf(stderr, " %d:%d", mine.units[u].op, mine.units[u].arg);
            std::fprintf(stderr, "\n");
        }
        if (diag.successful_unit_actions != diag.requested_unit_actions) {
            r.why = "unit action fails at hour " + std::to_string(h);
            r.fail_hour = h;
            if (std::getenv("DC11_FUNDDEBUG"))  // name the first failing unit (units act in list order)
                for (int u = 0; u < mine.n_units; ++u) {
                    Action one;
                    one.n_units = mine.n_units;
                    for (int v = 0; v <= u; ++v) one.units[v] = mine.units[v];
                    one.finalize();
                    const auto d = (me == 0 ? sim.diagnose_joint_actions(one, other) : sim.diagnose_joint_actions(other, one)).players[me];
                    if (d.successful_unit_actions == d.requested_unit_actions) continue;
                    const Farm& f = sim.st.farms[me];
                    const Tile& t = f.tiles[f.pos_y[u]][f.pos_x[u]];
                    std::fprintf(stderr, "fundfail h%d unit %d op %d arg %d at %d kind %d what %d yield %d animal %d | planned op %d arg %d\n", h, u,
                                 mine.units[u].op, mine.units[u].arg, f.pos_y[u] * BOARD + f.pos_x[u], t.kind, t.what, t.yield_units, t.has_animal,
                                 u < plan.actions[h].n_units ? plan.actions[h].units[u].op : -1, u < plan.actions[h].n_units ? plan.actions[h].units[u].arg : -1);
                    break;
                }
            return r;
        }
        // Planned orders the executor dropped (cash guard) or the market did not fill.
        const Action& planned = plan.actions[h];
        for (int k = 0; k < planned.n_orders; ++k) {
            const auto& o = planned.orders[k];
            int filled = -1;
            for (int j = 0; j < mine.n_orders && filled < 0; ++j)
                if (mine.orders[j].op == o.op && mine.orders[j].item == o.item)
                    filled = accepted.orders[j].op == o.op ? accepted.orders[j].n : 0;
            const int want = (o.op == M_HIRE || o.op == M_BUY_LAND) ? 1 : o.n;
            int hires_submitted = 0, hires_filled = 0;
            if (o.op == M_HIRE) {
                for (int j = 0; j < mine.n_orders; ++j)
                    if (mine.orders[j].op == M_HIRE) ++hires_submitted, hires_filled += accepted.orders[j].op == M_HIRE && accepted.orders[j].n > 0;
                if (hires_filled == hires_submitted && hires_submitted > 0) continue;
                if (options.hire_check && hires_filled < hires_submitted) {  // hirecheck: a partial hire wave fails the plan
                    r.why = "hires " + std::to_string(hires_filled) + " of " + std::to_string(hires_submitted) + " at hour " + std::to_string(h);
                    r.fail_hour = h, r.fail_op = M_HIRE, r.fail_item = o.item, r.hires_filled = hires_filled + sim.st.farms[me].hires_today;
                    return r;
                }
            }
            // cashsell: a short seed order does not fail the plan (the executor buys what the cash covers and plants what is in stock)
            if (filled < want && o.op == M_BUY_SEED && options.cash_sell && options.cash_seeds) continue;
            if (o.op == M_BUY_ANIMAL && options.buy_ahead && std::max(0, filled) + executor.consumed(h, o.item - GOOSE) >= want) continue;
            if (filled < want) {
                r.why = "order " + std::to_string(int(o.op)) + " item " + std::to_string(int(o.item)) + " short at hour " + std::to_string(h);
                r.fail_hour = h, r.fail_op = o.op, r.fail_item = o.item;
                return r;
            }
        }
        me == 0 ? sim.step(mine, other) : sim.step(other, mine);
    }
    if (options.dawn_reserve && dawn.day >= 1 && dawn.day < LAST_DAY && plan.fallback < SurvivalOnly) {
        int animals = 0;
        for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(dawn, cell).has_animal;
        for (int a = 0; a < N_ANIMALS; ++a) animals += plan.new_animals[a];
        const Farm& end = sim.st.farms[me];
        const double price = market_price(WHEAT, sim.st.market.inventory[WHEAT] - 1);
        int own_wheat = end.shed[WHEAT];
        if (options.reserve_net) {  // reservenet: tomorrow's feed net of our own wheat (pockets tonight, harvestable wheat tomorrow)
            for (int u = 0; u < end.n_units; ++u) own_wheat += end.inv[u][WHEAT];
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    const Tile& t = end.tiles[y][x];
                    const int age = dawn.day + 1 - t.planted_day;
                    if (t.kind == T_PLANT && t.what == WHEAT && age >= CROPS[WHEAT].first_yield_day && age <= CROPS[WHEAT].max_yield_day)
                        own_wheat += t.yield_units;
                }
        }
        const double reserve = std::max(0, animals - own_wheat) * price * 1.2 + (options.reserve_net ? 0.0 : options.reserve_slack);
        double assets = end.money;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            if (p == WHEAT) continue;
            int units = end.shed[p];
            for (int u = 0; u < end.n_units; ++u) units += end.inv[u][p];
            assets += 0.8 * units * sim.st.market.prices[p];
        }
        if (assets < reserve && end.money < dawn.self().money) {
            r.why = "end assets " + std::to_string(int(assets)) + " below next-dawn reserve " + std::to_string(int(reserve));
            r.reserve = true;
            return r;
        }
    }
    const Farm& end = sim.st.farms[me];
    r.skipped_seed = executor.skipped_seed();
    r.value = end.money - options.market.rival_weight * sim.st.farms[1 - me].money;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        int units = end.shed[p];
        for (int u = 0; u < end.n_units; ++u) units += end.inv[u][p];
        r.value += options.market.hold_discount * sale_value(p, sim.st.market.inventory[p], units);
    }
    r.ok = true;
    return r;
}

// cashsell: the first hour the land fills when it is ordered every hour from `from` on and the plan's purchases from then on wait
// (the seller funds it first); -1: not today.
// dc12 (affordanimal): the same for one animal of species `item` (GOOSE / COW / SHEEP): the first hour it fills when ordered
// every hour from `from` before the plan's later purchases (M&M buys the day-3 cow at h8 from the morning's sales).
int afford_hour(const agent::AgentObservation& dawn, const History& history, const Plan& plan, const Options& options,
                const double rival[HOURS][N_PRODUCTS], int from, int op, int item) {
    Plan p = plan;
    for (int h = from; h < p.hours; ++h) {
        Action& a = p.actions[h];
        int kept = 0;
        for (int k = 0; k < a.n_orders; ++k)
            if (a.orders[k].op == M_HIRE || a.orders[k].op == M_SELL) a.orders[kept++] = a.orders[k];
        if (kept < 10) a.orders[kept++] = {uint8_t(op), uint8_t(op == M_BUY_LAND ? 0 : item), 1};
        a.n_orders = kept;
    }
    Config config;
    Sim sim = sim_from_observation(dawn, config);
    Executor executor;
    struct Quiet { Quiet() { g_sell_quiet = true; } ~Quiet() { g_sell_quiet = false; } } quiet;  // DC11_AUDIT: live hours only
    Options simulated = options;
    simulated.market.intraday = nullptr;
    simulated.market.carry_target = nullptr;
    simulated.market.next_morning = nullptr;  // local addition (nextm)
    simulated.market.scenarios = 0;
    simulated.sell_model = nullptr, simulated.sell_tracker = nullptr;  // sellmodel: live executor only
    executor.start(dawn, history, p, simulated);
    const int me = dawn.player, quadrants = dawn.self().n_quadrants;
    auto owned = [&](const Farm& f) {  // animals of species `item`: in the shed, carried, placed
        int n = f.shed[item];
        for (int u = 0; u < f.n_units; ++u) n += f.inv[u][item];
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) n += f.tiles[y][x].has_animal && f.tiles[y][x].what == item;
        return n;
    };
    for (int h = 0; h < p.hours; ++h) {
        const int before = op == M_BUY_LAND ? 0 : owned(sim.st.farms[me]);
        Action mine;
        executor.act(agent::runtime::make_observation(sim, me), history, mine);
        Action other;
        Farm& opponent = sim.st.farms[1 - me];
        other.n_units = opponent.n_units;
        for (int q = 0; q < N_PRODUCTS; ++q) {
            const int n = int(std::lround(std::max(0.0, rival[h][q])));
            if (!n || other.n_orders >= 10) continue;
            opponent.shed[q] = Count(n);
            other.orders[other.n_orders++] = {M_SELL, uint8_t(q), n};
        }
        other.finalize();
        me == 0 ? sim.step(mine, other) : sim.step(other, mine);
        if (op == M_BUY_LAND ? sim.st.farms[me].n_quadrants > quadrants : h >= from && owned(sim.st.farms[me]) > before) return h;
    }
    return -1;
}
int land_afford_hour(const agent::AgentObservation& dawn, const History& history, const Plan& plan, const Options& options,
                     const double rival[HOURS][N_PRODUCTS], int from) {
    return afford_hour(dawn, history, plan, options, rival, from, M_BUY_LAND, 0);
}

// ---------------------------------------------------------------- trims
bool drop_animal(const Schema& s, DayIntent& in, int a) {
    --in.new_animal[a];
    const int gi = s.new_animal_group[a];
    in.feed[gi] = std::min(in.feed[gi], in.new_animal[a]);
    in.care[gi] = std::min(in.care[gi], in.feed[gi]);
    if (in.new_animal[a] + in.reserve[a] < s.unplaced_dawn[a]) ++in.reserve[a];  // never discard
    return true;
}
bool drop_crop(const Schema& s, DayIntent& in, int c) {
    --in.new_crop[c];
    if (!CROPS[c].ongoing) {
        auto& opts = in.options[s.new_crop_group[c]];
        *std::max_element(opts.begin(), opts.end()) -= 1;
    }
    return true;
}
// One unit of the network's least-wanted (trimnet) or the most expensive new entity (days before cropfirst: crops before animals).
bool trim_new_entity(const Schema& s, DayIntent& in, const Options& options) {
    if (options.trim_net)
        while (in.trim_next < in.trim_count) {
            const int i = in.trim_order[in.trim_next++];
            if (i < N_CROPS ? in.new_crop[i] > 0 : in.new_animal[i - N_CROPS] > 0)
                return i < N_CROPS ? drop_crop(s, in, i) : drop_animal(s, in, i - N_CROPS);
        }
    auto animal = [&] {
        for (int a : {2, 1, 0})
            if (in.new_animal[a] > 0) return drop_animal(s, in, a);
        return false;
    };
    auto crop = [&] {
        int order[N_CROPS] = {0, 1, 2, 3, 4};
        std::sort(order, order + N_CROPS, [](int a, int b) { return CROPS[a].seed > CROPS[b].seed; });
        for (int c : order)
            if (in.new_crop[c] > 0) return drop_crop(s, in, c);
        return false;
    };
    if (s.day < options.crop_first) return crop() || animal();
    return animal() || crop();
}

// ---------------------------------------------------------------- one compile of one intent
// Routes and realizes a bound intent; spawn tiles and the first-hour hire count are corrected
// from the engine replay and routed again.
bool route_and_realize(const agent::AgentObservation& dawn, Bound& b, int max_hires, Plan& plan, std::string& why, Plan& prof) {
    Problem& pb = b.pb;
    pb.max_hires = max_hires;
    RouteOut out;
    Realized z;
    for (int iteration = 0; iteration < 4; ++iteration) {
        double t = now_ms();
        out = route_day(pb);
        prof.ms_route += now_ms() - t;
        plan.evaluations += out.evaluations;
        prof.evaluations += out.evaluations;
        t = now_ms();
        z = realize(dawn, b, out);
        prof.ms_realize += now_ms() - t;
        if (!z.ok && z.first_wave >= 0) {
            pb.first_wave = z.first_wave;
            continue;
        }
        break;
    }
    if (!z.ok) {
        why = z.why;
        return false;
    }
    // saleslots=3: the dawn sales the seller wants take first-hour slots from hires only when the later start costs no work:
    // route again with the smaller first wave and keep it if the crew is the same and no required stop is dropped.
    if (b.sale_want > 0) {
        const int want = b.sale_want + (b.dawn_wheat_sale > 0);
        const int wave0 = std::min(out.hires, pb.first_wave), displaced = want - std::max(0, 10 - z.first_orders - wave0);
        auto required = [](const RouteOut& o, const Realized& r) {
            int n = int(r.unfinished.size());
            for (int d : o.dropped) n += !(d >= 0 && o.stops[d].priority == P_EXTRA && o.stops[d].entity < 0);
            return n;
        };
        if (displaced > 0 && wave0 > 0) {
            Bound late = b;
            late.pb.first_wave = std::max(0, wave0 - displaced);
            late.sale_slots = want;
            double t = now_ms();
            const RouteOut out2 = route_day(late.pb);
            prof.ms_route += now_ms() - t;
            plan.evaluations += out2.evaluations;
            prof.evaluations += out2.evaluations;
            t = now_ms();
            const Realized z2 = realize(dawn, late, out2);
            prof.ms_realize += now_ms() - t;
            if (z2.ok && out2.hires == out.hires && required(out2, z2) <= required(out, z)) b = late, out = out2, z = z2;
        }
    }
    plan.hours = pb.last_hour + 1;
    std::copy_n(z.actions, HOURS, plan.actions);
    std::copy_n(&z.receipts[0][0], HOURS * N_PRODUCTS, &plan.receipts[0][0]);
    plan.night_carried = z.night_carried;
    std::copy_n(z.night_items, N_PRODUCTS, plan.night_items);
    plan.hires = out.hires;
    plan.dawn_wheat_sale = b.dawn_wheat_sale;
    std::copy_n(b.new_animals, N_ANIMALS, plan.new_animals);
    plan.dropped = int(z.unfinished.size());
    for (int d : out.dropped) (d >= 0 && out.stops[d].priority == P_EXTRA && out.stops[d].entity < 0 ? plan.skipped : plan.dropped) += 1;
    plan.moves = out.moves, plan.unit_actions = out.actions, plan.waits = out.waits;
    auto collect = [](const Stop& st) { return st.group >= 1000 && st.group < 3000 && st.product >= 0; };
    for (const Stop& st : pb.stops) plan.collects += collect(st);
    for (int d : out.dropped) plan.collects_dropped += d >= 0 && collect(out.stops[d]);
    int seen[BOARD * BOARD]{};
    for (const auto& route : out.routes)
        for (int k = 0, prev = -1; k < int(route.size()); ++k) {
            const Stop& st = out.stops[route[k]];
            if (!st.shed && st.tile != prev) ++plan.visits, ++seen[st.tile];
            prev = st.shed ? -1 : st.tile;
        }
    for (int c : seen) plan.tiles_worked += c > 0, plan.revisits += c > 1;
    for (int d : out.dropped) {
        const int e = d < 0 ? -1 - d : out.stops[d].entity;
        if (e >= 0 && is_animal(out.entities[e].item)) --plan.new_animals[out.entities[e].item - GOOSE];
        why += d < 0 ? " e" + std::to_string(out.entities[e].item)
                     : " t" + std::to_string(out.stops[d].tile) + "p" + std::to_string(out.stops[d].priority) + "o" +
                           std::to_string(out.stops[d].steps[0].op);
    }
    for (int r : z.unfinished) why += " route" + std::to_string(r) + "@" + std::to_string(out.starts[r].tile) + "h" + std::to_string(out.starts[r].hour);
    why += z.why;
    if (!why.empty()) plan.reason += "dropped" + why + ";";
    return true;
}

int hire_cap(const agent::AgentObservation& dawn, const Options& options, int wheat_sale = 0) {
    double cash = dawn.self().money + 0.9 * sale_value(WHEAT, dawn.market.inventory[WHEAT], wheat_sale);  // + dawn shed stock sellable at hour 0
    for (int p = 0; p < N_PRODUCTS; ++p)
        if (p != WHEAT && p != FERTILIZER) cash += 0.9 * sale_value(p, dawn.market.inventory[p], dawn.own.shed[p]);
    int affordable = 0;
    for (double money = cash; affordable < options.max_hires && money >= fib(affordable);) money -= fib(affordable++);
    return affordable;
}

// An unaffordable purchase at `hour`: buy that item two hours later (the router waits for it).
bool delay_purchase(Bound& b, int op, int item, int hour) {
    int8_t* from = op == M_BUY_SEED ? &b.pb.seed_from[item]
                   : op == M_BUY_ANIMAL ? &b.pb.buy_from[CG + item - GOOSE]
                   : op == M_BUY_PRODUCT && item == WHEAT ? &b.pb.buy_from[CW]
                   : op == M_BUY_PRODUCT && item == FERTILIZER ? &b.pb.buy_from[CF]
                                                               : nullptr;
    const int next = hour + 3;
    if (!from || next > b.pb.last_hour - 1 || *from >= next) return false;
    *from = int8_t(next);
    return true;
}


// Funding variant: the purchases of `plain` that dawn cash (money + sellable
// shed stock, after the hires) cannot cover, in survival order (wheat, fertilizer, seeds, land,
// animals), are bought from hour `from`; deposits before then earn a financing bonus.
void defer_purchases(Bound& b, const Plan& plain, const agent::AgentObservation& dawn, int from, int seed_lag = 0, bool animal_first = false) {
    double cash = dawn.self().money + 0.9 * sale_value(WHEAT, dawn.market.inventory[WHEAT], b.dawn_wheat_sale);
    for (int p = 0; p < N_PRODUCTS; ++p)
        if (p != WHEAT && p != FERTILIZER) cash += 0.9 * sale_value(p, dawn.market.inventory[p], dawn.own.shed[p]);
    for (int k = 0; k < plain.hires; ++k) cash -= fib(k);
    struct Buy { int rank, hour, op, item; double cost; };
    std::vector<Buy> buys;
    for (int h = 0; h < plain.hours; ++h)
        for (int k = 0; k < plain.actions[h].n_orders; ++k) {
            const auto& o = plain.actions[h].orders[k];
            if (o.op != M_BUY_SEED && o.op != M_BUY_ANIMAL && o.op != M_BUY_PRODUCT && o.op != M_BUY_LAND) continue;
            // dc12: animals are funded right after feed wheat (M&M buys the day-3 cow at h8, before the seeds); was rank 4 (last)
            // seedlag: animals are funded right after feed wheat, before the seeds (M&M buys the day-3 cow at h6-10, the seeds after)
            const int rank = o.op == M_BUY_PRODUCT ? (o.item == WHEAT ? 0 : 2)
                             : o.op == M_BUY_ANIMAL ? (animal_first || std::getenv("DC12_ANIMALFIRST") || seed_lag > 0 ? 1 : 5) : o.op == M_BUY_SEED ? 3 : 4;
            const double cost = o.op == M_BUY_SEED ? double(CROPS[o.item].seed) * o.n
                                : o.op == M_BUY_ANIMAL ? double(ANIMALS[o.item - GOOSE].cost) * o.n
                                : o.op == M_BUY_LAND ? double(LAND_PRICES[std::min(2, dawn.self().n_quadrants - 1)])
                                                     : sale_value(o.item, dawn.market.inventory[o.item] - o.n, o.n);
            buys.push_back({rank, h, o.op, o.item, cost});
        }
    std::stable_sort(buys.begin(), buys.end(), [](const Buy& x, const Buy& y) { return x.rank != y.rank ? x.rank < y.rank : x.hour < y.hour; });
    for (const Buy& buy : buys) {
        if (buy.cost <= cash) {
            cash -= buy.cost;
            continue;
        }
        if (buy.op == M_BUY_LAND) {
            set_land_hour(b, dawn, std::max(b.land_hour, from));
            cash = -1e18;
            continue;
        }
        int8_t* at = buy.op == M_BUY_SEED ? &b.pb.seed_from[buy.item]
                     : buy.op == M_BUY_ANIMAL ? &b.pb.buy_from[CG + buy.item - GOOSE]
                                              : &b.pb.buy_from[buy.item == WHEAT ? CW : CF];
        *at = int8_t(std::max<int>(*at, from + 1 + (buy.op == M_BUY_SEED ? seed_lag : 0)));
        cash = -1e18;  // later purchases wait too (they come after this one)
    }
    for (int p = 0; p < N_PRODUCTS; ++p)
        for (int h = 0; h < from + seed_lag && h < HOURS; ++h) b.pb.gain[p][h] += 0.5 * dawn.market.prices[p];
}

// affordall (dc12): each purchase of the plain plan gets its own earliest affordable hour on the day's cash timeline: dawn money
// (+ the wheatcash sale) minus the hires, plus 0.9 x the sale value of the shed stock by each hour (dawn stock + the plain plan's
// deposits). Purchases are taken in priority order (feed wheat, animals, bought products, land, seeds: M&M buys the day-3 cow at h8
// and the seeds after), each after the ones before it; the dependent pickups wait for the release hour. Deposits before the last
// release earn half their price on top (as the deferral variants).
void afford_all(Bound& b, const Plan& plain, const agent::AgentObservation& dawn) {
    double base = dawn.self().money + 0.9 * sale_value(WHEAT, dawn.market.inventory[WHEAT], b.dawn_wheat_sale);
    for (int k = 0; k < plain.hires; ++k) base -= fib(k);
    auto cash_at = [&](int h) {  // most cash raisable by hour h
        double c = base;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            if (p == WHEAT) continue;
            const int units = dawn.own.shed[p] + (h >= 0 ? plain.receipts[std::min(h, HOURS - 1)][p] : 0);
            c += 0.9 * sale_value(p, dawn.market.inventory[p], units);
        }
        return c;
    };
    struct Buy { int rank, hour, op, item; double cost; };
    std::vector<Buy> buys;
    for (int h = 0; h < plain.hours; ++h)
        for (int k = 0; k < plain.actions[h].n_orders; ++k) {
            const auto& o = plain.actions[h].orders[k];
            if (o.op != M_BUY_SEED && o.op != M_BUY_ANIMAL && o.op != M_BUY_PRODUCT && o.op != M_BUY_LAND) continue;
            const int rank = o.op == M_BUY_PRODUCT ? (o.item == WHEAT ? 0 : 2) : o.op == M_BUY_ANIMAL ? 1 : o.op == M_BUY_LAND ? 3 : 4;
            const double cost = o.op == M_BUY_SEED ? double(CROPS[o.item].seed) * o.n
                                : o.op == M_BUY_ANIMAL ? double(ANIMALS[o.item - GOOSE].cost) * o.n
                                : o.op == M_BUY_LAND ? double(LAND_PRICES[std::min(2, dawn.self().n_quadrants - 1)])
                                                     : sale_value(o.item, dawn.market.inventory[o.item] - o.n, o.n);
            buys.push_back({rank, h, o.op, o.item, cost});
        }
    std::stable_sort(buys.begin(), buys.end(), [](const Buy& x, const Buy& y) { return x.rank != y.rank ? x.rank < y.rank : x.hour < y.hour; });
    double committed = 0;
    int last = 0;
    for (const Buy& buy : buys) {
        committed += buy.cost;
        int at = buy.hour;
        while (at < plain.hours - 1 && cash_at(at) < committed) ++at;
        if (at <= buy.hour) continue;  // affordable at its planned hour
        last = std::max(last, at);
        if (buy.op == M_BUY_LAND) {
            set_land_hour(b, dawn, std::max(b.land_hour, at));
            continue;
        }
        int8_t* from = buy.op == M_BUY_SEED ? &b.pb.seed_from[buy.item]
                       : buy.op == M_BUY_ANIMAL ? &b.pb.buy_from[CG + buy.item - GOOSE]
                                                : &b.pb.buy_from[buy.item == WHEAT ? CW : CF];
        *from = int8_t(std::max<int>(*from, at + 1));
    }
    for (int p = 0; p < N_PRODUCTS; ++p)
        for (int h = 0; h < last && h < HOURS; ++h) b.pb.gain[p][h] += 0.5 * dawn.market.prices[p];
}

// Compiles one intent at one fallback level. Funding: the plain plan, then variants where the
// purchases dawn cash cannot cover wait for a return hour (3, 6, 10, 14) financed by earlier
// deposits; each variant gets one more repair (the failing purchase two hours later). A plan
// funded only under the expected forecast is kept when no variant is stress-funded.
Plan compile_level(const agent::AgentObservation& dawn, const History& history, const Schema& schema, const DayIntent& intent,
                   int level, const Options& options, Plan& prof) {
    Bound base = bind(dawn, schema, intent, level, options);
    base.land_first = options.cash_sell;
    const int hours = base.pb.last_hour + 1;
    const DayMarket market = day_market(dawn, history, options.market);
    if (options.sale_slots >= 2) {  // saleslots=2: slots only for the thin products the seller sells at the dawn (its h0 decision)
        static const int none[HOURS][N_PRODUCTS]{};
        int keep[N_PRODUCTS]{}, sell[N_PRODUCTS];
        keep[WHEAT] = dawn.own.shed[WHEAT], keep[FERTILIZER] = dawn.own.shed[FERTILIZER];
        choose_sales(dawn, none, hours, market, keep, 100, sell, nullptr, nullptr);
        // saleslots=4: as 2 (the first hire wave always leaves the dawn sales their slots, M&M's order: sells in slot 0, hires fill the
        // rest, the overflow hires the next hour), with the decision of the seller that sells: the learned model when it runs today.
        const bool model = options.sale_slots == 4 && options.sell_model && dawn.day >= options.sell_model_first &&
                           dawn.day <= options.sell_model_last && (options.sell_model_mode <= 2 || options.sell_model_mode == 5);
        if (model) {
            int units[N_PRODUCTS]{}, zero[N_PRODUCTS]{};
            if (options.sell_tracker) options.sell_tracker->update(dawn);  // the executor's own update this hour is then a no-op
            bcsell::sell_units(*options.sell_model, dawn, history, zero, options.sell_seed, units, true, options.sell_tracker);
            // as the executor at h0: the model's thin lots (raised to the DP's when tonight's room binds), the DP's other products
            for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) sell[p] = std::max(sell[p], std::min<int>(units[p], dawn.own.shed[p]));
        }
        int& n = options.sale_slots == 2 || options.sale_slots == 4 ? base.sale_slots : base.sale_want;
        for (int p = 0; p < N_PRODUCTS; ++p) n += sell[p] > 0;
        base.wheat_slot = options.sale_slots == 2 || options.sale_slots == 4;
    }
    // Deposit values; the opponent's sale hours are uncertain: averaged over scenarios where its
    // expected daily volume lands early (hours 1-7), mid-day (7-13) or late (14-20).
    auto values = [&](const int output[N_PRODUCTS], double out[N_PRODUCTS][HOURS]) {
        deposit_values(dawn, market, output, hours, out);
        if (options.timing_mix <= 0) return;
        double mixed[N_PRODUCTS][HOURS]{};
        for (int block = 0; block < 3; ++block) {
            DayMarket scenario = market;
            timing_scenario(market.rival, 0, hours, block, scenario.rival, options.timing_shift, options.timing_length);
            double gain[N_PRODUCTS][HOURS];
            deposit_values(dawn, scenario, output, hours, gain);
            for (int p = 0; p < N_PRODUCTS; ++p)
                for (int h = 0; h < HOURS; ++h) mixed[p][h] += gain[p][h] / 3;
        }
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int h = 0; h < HOURS; ++h) out[p][h] = (1 - options.timing_mix) * out[p][h] + options.timing_mix * mixed[p][h];
    };
    values(base.output, base.pb.gain);
    if (std::getenv("DC12_GAINLOG") && level == 0)  // gainlog,day,product,output,price,gain at h0 h4 h8 h12 h16 h19 h21 h22
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (base.output[p] > 0)
                std::fprintf(stderr, "gainlog,%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", dawn.day, p, base.output[p], int(dawn.market.prices[p]),
                             base.pb.gain[p][0], base.pb.gain[p][4], base.pb.gain[p][8], base.pb.gain[p][12], base.pb.gain[p][16], base.pb.gain[p][19],
                             base.pb.gain[p][21], base.pb.gain[p][22]);
    if (options.dep_credit > 0)  // depcredit: morning deposits of thin products earn lambda x price on top (M&M deposits them by h2-8)
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT && (p != FERTILIZER || options.dep_fert))
                for (int h = 0; h <= options.dep_hour && h < HOURS; ++h) base.pb.gain[p][h] += options.dep_credit * dawn.market.prices[p];
    if (options.regime_on(dawn.day) && dawn.day < LAST_DAY) {  // regime M: carried products earn no deposit value before the evening, within
        int budget = base.pb.night_room - base.pb.feed_reserve - options.regime_room;  // tonight's room (the rest as before)
        int n = 0;
        for (int p : {int(WOOL), int(MILK), int(STRAWBERRY), int(TOMATO), int(EGG)})
            if (options.regime_carry >> p & 1 && n++ < options.regime_products() - options.regime_skip && base.output[p] <= budget) {
                budget -= base.output[p];
                std::fill_n(base.pb.gain[p], std::min(options.regime_eve, HOURS), 0.0);
            }
    }

    // cashsell: no dawn-cash cap (the seller raises the hires' cash at dawn; the funding check's hire repair limits the crew)
    int max_hires = options.cash_sell ? options.max_hires : std::min(options.max_hires, hire_cap(dawn, options));
    double stress[HOURS][N_PRODUCTS], expected[HOURS][N_PRODUCTS];
    stress_forecast(dawn, history, stress, options.stress_hour);
    history.expected(dawn.day, 3, expected);
    if (options.market.first_full && options.first_funding) first_sales(dawn, expected);
    std::string log;
    Plan plain, expected_plan;
    bool expected_ok = false, have_plain = false;
    int first_short = -1;  // cashroute: the plain plan's first unfunded hour
    auto attempt = [&](Bound& b, const char* label, int tries) {
        int land_moves = 0;  // cashsell: up to 2 land moves on top of the tries
        int hire_moves = 0;  // latehire: up to 3 later hire hours on top of the tries
        int animal_moves = 0;  // affordanimal: up to 2 animal moves on top of the tries
        for (int repair = 0; repair < tries; ++repair) {
            Plan plan;
            plan.fallback = level;
            std::string why;
            if (!route_and_realize(dawn, b, max_hires, plan, why, prof)) {
                log += "L" + std::to_string(level) + label + ":" + why + ";";
                plan.status = CompileStatus::NoSchedule;
                return plan;
            }
            if (dawn.day >= 1 && dawn.day <= options.feed_late && dawn.day < LAST_DAY) add_late_feed(dawn, plan);
            log += plan.reason;
            if (!have_plain) plain = plan, have_plain = true;
            const double t = now_ms();
            const Funding f = funded(dawn, history, plan, options, stress);
            prof.ms_fund += now_ms() - t;
            if (f.ok) {
                plan.status = CompileStatus::Ok;
                plan.stress_funded = true;
                plan.skipped_seed = f.skipped_seed;
                for (int pull = 0; land_moves > 0 && pull < options.land_pull && !options.spent(prof.evaluations); ++pull) {  // landpull
                    const int at = land_afford_hour(dawn, history, plan, options, stress, 0);
                    if (at < 0 || at >= b.land_hour) break;
                    Bound pulled = b;
                    set_land_hour(pulled, dawn, at);
                    Plan q;
                    q.fallback = level;
                    std::string pull_why;
                    if (!route_and_realize(dawn, pulled, max_hires, q, pull_why, prof)) break;
                    if (dawn.day >= 1 && dawn.day <= options.feed_late && dawn.day < LAST_DAY) add_late_feed(dawn, q);
                    if (!funded(dawn, history, q, options, stress).ok) break;
                    log += "pull " + std::to_string(b.land_hour) + "->" + std::to_string(at) + ";";
                    q.status = CompileStatus::Ok;
                    q.stress_funded = true;
                    b = pulled, plan = q;
                }
                return plan;
            }
            log += "L" + std::to_string(level) + label + std::to_string(repair) + ":stress " + f.why + ";";
            if (first_short < 0) first_short = f.fail_hour;
            if (!expected_ok && funded(dawn, history, plan, options, expected).ok) expected_ok = true, expected_plan = plan;
            plan.status = CompileStatus::Unfunded;
            if (f.reserve || f.fail_hour < 0) return plan;
            if (f.fail_op == M_BUY_LAND) {  // cashsell: the land at its first affordable hour
                if (!options.cash_sell || land_moves++ >= 2) return plan;
                const int at = land_afford_hour(dawn, history, plan, options, stress, f.fail_hour);
                if (at <= b.land_hour || at >= b.pb.last_hour - 1) return plan;
                set_land_hour(b, dawn, at);
                for (int p = 0; p < N_PRODUCTS; ++p)  // deposits before it finance it (as the deferral variants' bonus)
                    for (int h = 0; h < at; ++h) b.pb.gain[p][h] += 0.5 * dawn.market.prices[p];
                --repair;
            } else if (f.fail_op == M_BUY_ANIMAL && options.afford_animal && animal_moves++ < 2) {  // dc12: the animal at its first affordable hour
                const int at = afford_hour(dawn, history, plan, options, stress, f.fail_hour, M_BUY_ANIMAL, f.fail_item);
                int8_t& ready = b.pb.buy_from[CG + f.fail_item - GOOSE];
                if (at < 0 || at + 1 <= ready || at >= b.pb.last_hour - 2) {
                    if (!delay_purchase(b, f.fail_op, f.fail_item, f.fail_hour)) return plan;
                } else {
                    ready = int8_t(at + 1);
                    for (int p = 0; p < N_PRODUCTS; ++p)  // deposits before it finance it
                        for (int h = 0; h < at; ++h) b.pb.gain[p][h] += 0.5 * dawn.market.prices[p];
                    --repair;
                }
            } else if (f.fail_op == M_HIRE && options.late_hire && hire_moves < 3 && f.fail_hour + 2 < b.pb.last_hour - 4) {
                // latehire: the hires dawn cash cannot pay are ordered later, after the morning's sales (not dropped from the crew)
                ++hire_moves;
                int paid = 0;
                for (double c = dawn.self().money; paid < plan.hires && c >= fib(paid);) c -= fib(paid++);
                b.pb.cash_wave = std::min(b.pb.cash_wave, paid);
                b.pb.cash_hire_hour = std::max(b.pb.cash_hire_hour, f.fail_hour + 2);
                --repair;
            } else if (f.fail_op == M_HIRE && options.hire_check && f.hires_filled >= 0)  // hirecheck: the crew the cash covered
                max_hires = std::max(0, std::min({max_hires, plan.hires - 1, f.hires_filled}));
            else if (f.fail_op == M_HIRE) max_hires = std::max(0, std::min(max_hires, plan.hires) - 1);
            else if (!delay_purchase(b, f.fail_op, f.fail_item, f.fail_hour)) return plan;
        }
        Plan failed;
        failed.status = CompileStatus::Unfunded;
        return failed;
    };
    Plan plan = attempt(base, "F", 2);
    // achieve: every funded variant is a candidate; the one achieving most is kept (see Options::achieve)
    Plan achieved;
    bool have_achieved = false;
    std::string achieved_label;
    auto better = [&](const Plan& a, const Plan& b) {  // a achieves more than b
        double va = 0, vb = 0;
        for (int k = 0; k < N_ANIMALS; ++k) va += a.new_animals[k] * ANIMALS[k].cost, vb += b.new_animals[k] * ANIMALS[k].cost;
        if (options.achieve_mode == 3) {  // by value: a planted seed returns ~10x its price over its life, an animal ~2.5x
            const double sa = va - 4 * a.skipped_seed, sb = vb - 4 * b.skipped_seed;
            if (sa != sb) return sa > sb;
            return a.dropped < b.dropped;
        }
        if (va != vb) return va > vb;
        if (a.skipped_seed != b.skipped_seed) return a.skipped_seed < b.skipped_seed;
        return a.dropped < b.dropped;
    };
    auto consider = [&](const Plan& p, const std::string& lab) {
        if (p.status != CompileStatus::Ok) return;
        int animals = 0, plants = 0;
        for (int k = 0; k < N_ANIMALS; ++k) animals += p.new_animals[k];
        for (int h = 0; h < p.hours; ++h)
            for (int u = 0; u < p.actions[h].n_units; ++u) plants += p.actions[h].units[u].op == OP_PLANT;
        log += lab + " ok skip " + std::to_string(int(p.skipped_seed)) + " drop " + std::to_string(p.dropped) + " animals " + std::to_string(animals) +
               " plants " + std::to_string(plants) + ";";
        if (!have_achieved || better(p, achieved)) achieved = p, have_achieved = true, achieved_label = lab;
    };
    // achieve=2: only seed-cash shortfalls (skipped plantings) or unfunded plans trigger the search; a funded plan that only dropped a
    // stop is kept as before (mid-game days otherwise switched to deferral variants: G3 exact worlds -1.1k)
    auto complete = [&](const Plan& p) {
        return p.status == CompileStatus::Ok && p.skipped_seed == 0 && (p.dropped == 0 || options.achieve_mode >= 2);
    };
    if (options.achieve && plan.status == CompileStatus::Ok && !complete(plan)) consider(plan, "F");
    else if (plan.status != CompileStatus::Unfunded) {
        plan.reason = log;
        return plan;
    }
    if (options.afford_all && have_plain && !options.spent(prof.evaluations)) {  // affordall: per-purchase release hours
        Bound b = base;
        const bool wheat = options.wheat_cash && dawn.own.shed[WHEAT] > 0;
        if (wheat) b.dawn_wheat_sale = dawn.own.shed[WHEAT], b.pb.dawn_stock[CW] = 0, b.pb.lazy_wheat = true, b.pb.wheat_buy_cost = options.feed_cost * dawn.market.prices[WHEAT];
        afford_all(b, plain, dawn);
        const int hires_before = max_hires;
        if (wheat) max_hires = std::max(max_hires, std::min(options.max_hires, hire_cap(dawn, options, b.dawn_wheat_sale)));
        plan = attempt(b, "A", 2);
        max_hires = hires_before;
        if (plan.status == CompileStatus::Ok) {
            plan.reason = log;
            return plan;
        }
    }
    // cashroute: the cash need's shadow price in the deposit values. The purchases stay at their hours; every unit deposited by
    // the first short hour earns lambda x its price on top (the seller sells it for the purchase), and the day is routed again.
    if (options.cash_route > 0 && first_short >= 0 && !options.spent(prof.evaluations)) {
        Bound b = base;
        const bool wheat = options.wheat_cash && dawn.own.shed[WHEAT] > 0;
        if (wheat) b.dawn_wheat_sale = dawn.own.shed[WHEAT], b.pb.dawn_stock[CW] = 0, b.pb.lazy_wheat = true, b.pb.wheat_buy_cost = options.feed_cost * dawn.market.prices[WHEAT];
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int h = 0; h <= first_short && h < HOURS; ++h) b.pb.gain[p][h] += options.cash_route * dawn.market.prices[p];
        const int hires_before = max_hires;
        if (wheat) max_hires = std::max(max_hires, std::min(options.max_hires, hire_cap(dawn, options, b.dawn_wheat_sale)));
        plan = attempt(b, "C", 2);
        max_hires = hires_before;
        if (plan.status == CompileStatus::Ok) {
            plan.reason = log;
            return plan;
        }
    }
    // cashshadow (with achieve): cash has a shadow price while the day's purchases are cash-bound. Variants whose deposits before the
    // short hour (first_short, else h6 / h10) earn lambda x price on top (0.5 / 1), with and without the dawn wheat sale, compete in
    // the achieve search: routes bring sellable stock to the shed before the purchases need the cash (M&M's day-1 fertilizer loop).
    if (options.achieve && options.cash_shadow && (!have_achieved || !complete(achieved)) && !options.spent(prof.evaluations)) {
        std::vector<int> horizons;
        if (first_short >= 0) horizons.push_back(first_short);
        else horizons = {6, 10};
        for (const int until : horizons)
            for (const double lambda : {0.5, 1.0})
                for (int wheat = 0; wheat <= (options.wheat_cash && dawn.own.shed[WHEAT] > 0); ++wheat) {
                    if (options.spent(prof.evaluations)) break;
                    Bound b = base;
                    if (wheat) b.dawn_wheat_sale = dawn.own.shed[WHEAT], b.pb.dawn_stock[CW] = 0, b.pb.lazy_wheat = true, b.pb.wheat_buy_cost = options.feed_cost * dawn.market.prices[WHEAT];
                    for (int p = 0; p < N_PRODUCTS; ++p)
                        for (int h = 0; h <= until && h < HOURS; ++h) b.pb.gain[p][h] += lambda * dawn.market.prices[p];
                    const int hires_before = max_hires;
                    if (wheat) max_hires = std::max(max_hires, std::min(options.max_hires, hire_cap(dawn, options, b.dawn_wheat_sale)));
                    const std::string label = "S" + std::to_string(int(lambda * 10)) + "h" + std::to_string(until) + (wheat ? "w" : "");
                    Plan q = attempt(b, label.c_str(), 2);
                    max_hires = hires_before;
                    consider(q, label);
                }
    }
    // lazyfeed: when dawn cash and stock cannot pay the plan's first feed-wheat purchase (a $0 dawn), the deferral variants fetch
    // the wheat at the first feed, after the trips whose deposits pay for it (otherwise each trip waits at the shed for it and
    // nothing is sold first)
    bool lazy_feed = false;
    if (options.lazy_feed && have_plain) {
        double cash = dawn.self().money, wheat = 0;
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT) cash += 0.9 * sale_value(p, dawn.market.inventory[p], dawn.own.shed[p]);
        for (int h = 0; h < plain.hours && !wheat; ++h)
            for (int k = 0; k < plain.actions[h].n_orders; ++k)
                if (const auto& o = plain.actions[h].orders[k]; o.op == M_BUY_PRODUCT && o.item == WHEAT)
                    wheat = sale_value(WHEAT, dawn.market.inventory[WHEAT] - o.n, o.n);
        lazy_feed = cash < wheat;
    }
    Plan best_variant;  // bestvariant: the funded deferral variant with the fewest dropped required stops (earliest on ties)
    bool have_best = false;
    bool achieved_complete = false;  // achieve: a variant with nothing skipped or dropped ends the search
    const int lags = options.achieve && options.seed_lag ? 3 : 1;  // seedlag: seeds released 0 / 4 / 8 h after the animals
    // animalfirst=2 (with achieve): both funding orders (seeds before animals, animals before seeds) are variants; the search keeps
    // the one achieving more (day 9: a big sheep ask funded after the seeds lost its pastures; day 6: the new quadrant's seeds after
    // the animals lost plantings)
    const int orders = options.achieve && options.animal_first_mode == 2 ? 2 : 1;
    for (const int from : {3, 6, 10, 14})
     for (int order = 0; order < orders; ++order)
      for (int lag_i = 0; lag_i < lags; ++lag_i)
        for (int wheat = 0; wheat <= (options.wheat_cash && dawn.own.shed[WHEAT] > 0) && !achieved_complete; ++wheat) {
            if (options.spent(prof.evaluations)) break;
            const int lag = 4 * lag_i;
            if (lag && from + 1 + lag > 14) break;  // later seed releases plant too late to water them today (weeds)
            Bound b = base;
            if (wheat) b.dawn_wheat_sale = dawn.own.shed[WHEAT], b.pb.dawn_stock[CW] = 0, b.pb.lazy_wheat = true, b.pb.wheat_buy_cost = options.feed_cost * dawn.market.prices[WHEAT];
            defer_purchases(b, plain, dawn, from, lag, orders == 2 ? order == 1 : options.animal_first);
            if (lazy_feed) b.pb.lazy_wheat = true;
            if (wheat) b.pb.buy_from[CW] = int8_t(std::max<int>(b.pb.buy_from[CW], from));
            const std::string label = "F" + std::to_string(from) + (lag ? "s" + std::to_string(lag) : "") + (orders == 2 && order ? "a" : "") + (wheat ? "w" : "") + "r";
            const int hires_before = max_hires;  // wheatcash: the sold wheat pays hires too
            if (wheat) max_hires = std::max(max_hires, std::min(options.max_hires, hire_cap(dawn, options, b.dawn_wheat_sale)));
            plan = attempt(b, label.c_str(), 2);
            if (wheat) max_hires = hires_before;
            if (options.achieve) {
                consider(plan, label);
                achieved_complete = complete(plan);
                continue;
            }
            if (plan.status == CompileStatus::Ok) {
                if (!options.best_variant) {
                    plan.reason = log;
                    return plan;
                }
                if (!have_best || plan.dropped < best_variant.dropped) best_variant = plan, have_best = true;
                if (!best_variant.dropped) break;  // nothing dropped: no later variant can do better on this measure
            }
        }
    if (have_best) {
        best_variant.reason = log;
        return best_variant;
    }
    if (have_achieved) {
        achieved.reason = log + "achieve " + achieved_label + " skipped " + std::to_string(int(achieved.skipped_seed)) + ";";
        return achieved;
    }
    if (expected_ok) {
        expected_plan.status = CompileStatus::Ok;
        expected_plan.stress_funded = false;
        expected_plan.reason = log;
        return expected_plan;
    }
    // survivalfloor: the last level never leaves an idle day. An unfunded survival plan (harvests, survival water, feeds capped by the
    // cash budget) still collects and sells today's products and feeds what the shed and the cash allow; the executor's guards drop
    // the purchases it cannot pay. Idle, the animals go unfed and escape after two days (a $10 dawn became a dead farm, -186k).
    if (options.survival_floor && level == SurvivalOnly && have_plain) {
        plain.status = CompileStatus::Ok;
        plain.stress_funded = false;
        plain.reason = log + "survival floor;";
        return plain;
    }
    plan.status = CompileStatus::Unfunded;
    plan.reason = log;
    return plan;
}
}


namespace {
Plan compile_day_once(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent, const Options& options);
}

// regime M: while tonight's projected carry home exceeds regimelimit, recompile with one carried product fewer (lowest value first).
Plan compile_day(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent, const Options& options) {
    Plan plan = compile_day_once(dawn, history, intent, options);
    if (!options.regime_on(dawn.day) || dawn.day >= LAST_DAY) return plan;
    Options fewer = options;
    while (plan.status == CompileStatus::Ok && plan.night_carried > options.regime_limit && fewer.regime_skip < options.regime_products()) {
        ++fewer.regime_skip;
        const double ms = plan.compile_ms;
        Plan p = compile_day_once(dawn, history, intent, fewer);
        if (p.status != CompileStatus::Ok) break;
        plan = p;
        plan.compile_ms += ms;
    }
    return plan;
}

namespace {
Plan compile_day_once(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent, const Options& options) {
    const double t0 = now_ms();
    Schema schema = describe(dawn);
    size_new_groups(schema, intent);
    Plan plan;
    if (const std::string invalid = validate(schema, intent); !invalid.empty()) {
        plan.status = CompileStatus::InvalidIntent;
        plan.reason = invalid;
        plan.compile_ms = now_ms() - t0;
        return plan;
    }
    std::string log;
    Plan prof;
    auto level_plan = [&](const DayIntent& in, int level) {
        Plan p = compile_level(dawn, history, schema, in, level, options, prof);
        log += p.reason;
        return p;
    };
    plan = level_plan(intent, KeepAll);
    // startcomplete: a day-0 plan that drops stops (entities bought late and never placed, extra hires, cash left unspent: the
    // starved opening) goes to the trims until a complete plan is funded; the dropping plan stays the fallback.
    const bool complete_day0 = dawn.day < options.day0_complete;  // startcomplete=N: days 0..N-1
    Plan dropping;
    if (complete_day0 && plan.status == CompileStatus::Ok && plan.dropped > 0) dropping = plan, plan.status = CompileStatus::NoSchedule;
    if (plan.status != CompileStatus::Ok && intent.buy_land && options.land_trims > 0) {
        DayIntent trimmed = intent;
        for (int trims = 1; trims <= options.land_trims && !options.spent(prof.evaluations) && trim_new_entity(schema, trimmed, options); ++trims) {
            Plan p = level_plan(trimmed, KeepAll);
            if (p.status == CompileStatus::Ok) {
                p.trims = trims;
                plan = p;
                break;
            }
        }
    }
    // landfirst: the land alone (today's other new entities dropped) before the NoLand level. A planting or an animal skipped today
    // is asked again tomorrow; a slipped quadrant cascades (hybrid + m1, league seed 505 vs D: Q2 d6 -> d8, Q3 d8 -> d10, and the
    // network never asks Q4 after day 10: -16.5k).
    if (plan.status != CompileStatus::Ok && intent.buy_land && options.land_first) {
        DayIntent only = intent;
        int dropped = 0;
        for (int c = 0; c < N_CROPS; ++c)
            while (only.new_crop[c] > 0) drop_crop(schema, only, c), ++dropped;
        for (int a = 0; a < N_ANIMALS; ++a)
            while (only.new_animal[a] > 0) drop_animal(schema, only, a), ++dropped;
        if (dropped > 0) {
            Plan p = level_plan(only, KeepAll);
            if (p.status == CompileStatus::Ok) {
                p.trims = dropped;
                p.fallback = std::max(p.fallback, int(NoNewEntities));  // never "complete": the agent's larger-intent tries must not take it (Day compiler fix)
                p.reason += "land first;";
                plan = p;
            }
        }
    }
    if (plan.status != CompileStatus::Ok && intent.buy_land) plan = level_plan(intent, NoLand);
    // Trims: one unit of the most expensive new entity at a time.
    if (plan.status != CompileStatus::Ok) {
        DayIntent trimmed = intent;
        for (int trims = 1; trims <= 12 && !options.spent(prof.evaluations) && trim_new_entity(schema, trimmed, options); ++trims) {
            Plan p = level_plan(trimmed, NoLand);
            if (p.status == CompileStatus::Ok && !(complete_day0 && p.dropped > 0)) {
                p.fallback = std::max(p.fallback, int(NoLand));  // compiled at the NoLand level (same plan without a land intent)
                p.trims = trims;
                if (!intent.buy_land) log += "trims without a land intent;";
                plan = p;
                break;
            }
        }
    }
    if (plan.status != CompileStatus::Ok && dropping.status == CompileStatus::Ok) plan = dropping;
    for (int level : {int(NoNewEntities), int(NoCollection), int(SurvivalOnly)}) {
        if (plan.status == CompileStatus::Ok) break;
        plan = level_plan(intent, level);
    }
    if (options.plan_menu && !options.regime_on(dawn.day) && plan.status == CompileStatus::Ok && plan.fallback == KeepAll && !plan.trims && dawn.day >= 6 &&
        dawn.day < LAST_DAY) {
        const DayMarket market = day_market(dawn, history, options.market);
        auto value = [&](const Plan& p) {  // day value against the forecast opponent
            const Funding f = funded(dawn, history, p, options, market.rival);
            return f.ok ? f.value : -1e100;
        };
        double best = value(plan);
        std::vector<Options> menu;
        auto early = [&](double keep) {
            Options o = options;
            o.market.wait_cost = 1 - keep, o.market.wait_route_only = true;
            menu.push_back(o);
        };
        if (options.plan_menu & 1) early(0.998);
        if (options.plan_menu & 2) early(0.995);
        for (size_t k = 0; k < menu.size() && !options.spent(prof.evaluations); ++k) {
            Plan p = compile_level(dawn, history, schema, intent, KeepAll, menu[k], prof);
            if (p.status != CompileStatus::Ok) continue;
            const double v = value(p);
            if (v > best + 1) best = v, plan = p, plan.choice = int(k) + 1;
        }
    }
    plan.reason = log;
    plan.ms_route = prof.ms_route, plan.ms_realize = prof.ms_realize, plan.ms_fund = prof.ms_fund;
    plan.evaluations = prof.evaluations;
    plan.compile_ms = now_ms() - t0;
    return plan;
}

}  // namespace

// ---------------------------------------------------------------- executor
void Executor::start(const agent::AgentObservation& dawn, const History& history, const Plan& plan, const Options& options) {
    plan_ = plan;
    options_ = options;
    day_ = dawn.day;
    market_ = day_market(dawn, history, options.market);
    std::fill_n(band_acc_, N_PRODUCTS, 0.0);
    std::fill_n(unplanted_, BOARD * BOARD, false);
    skipped_seed_ = 0;
    std::fill_n(sold_today_, N_PRODUCTS, 0);
    std::fill_n(ahead_, N_ANIMALS, 0);
    for (auto& c : consumed_) std::fill_n(c, N_ANIMALS, 0);
}

void Executor::act(const agent::AgentObservation& obs, const History& history, Action& action) {
    action.clear();
    action.n_units = obs.self().n_units;
    for (int u = 0; u < action.n_units; ++u) action.units[u] = UnitAction{};
    const int h = obs.hour;
    if (obs.day != day_ || h >= plan_.hours) {
        action.finalize();
        return;
    }
    if (options_.sell_tracker) options_.sell_tracker->update(obs);  // sellmodel level inputs: every hour
    static const bool crew_check = std::getenv("DC12_CREWCHECK") != nullptr;
    if (crew_check && live_seller) {  // crewcheck,player,day,hour,planned units,units,missing unit actions this hour (invariant: 0; m14b)
        int missing = 0;
        for (int u = obs.self().n_units; u < plan_.actions[h].n_units; ++u) missing += plan_.actions[h].units[u].op != OP_PASS;
        if (missing > 0)
            std::fprintf(stderr, "crewcheck,%d,%d,%d,%d,%d,%d\n", obs.player, obs.day, h, int(plan_.actions[h].n_units), int(obs.self().n_units), missing);
    }
    const Action& planned = plan_.actions[h];
    for (int u = 0; u < std::min(action.n_units, planned.n_units); ++u) action.units[u] = planned.units[u];
    if (options_.cash_sell) {  // the engine blocks all of a crop's plantings in a turn when they exceed its seeds: plant what is in stock
        int seeds[N_CROPS];
        for (int c = 0; c < N_CROPS; ++c) seeds[c] = obs.own.seeds[c];
        for (int u = 0; u < action.n_units; ++u)
            if (action.units[u].op == OP_PLANT && action.units[u].arg < N_CROPS && seeds[action.units[u].arg] > 0 && options_.skip_water)
                unplanted_[obs.self().pos_y[u] * BOARD + obs.self().pos_x[u]] = false;  // planted after an earlier skip there
        for (int u = 0; u < action.n_units; ++u)
            if (action.units[u].op == OP_PLANT && action.units[u].arg < N_CROPS && seeds[action.units[u].arg]-- <= 0) {
                skipped_seed_ += CROPS[action.units[u].arg].seed;
                action.units[u] = UnitAction{};
                if (options_.skip_water) unplanted_[obs.self().pos_y[u] * BOARD + obs.self().pos_x[u]] = true;
            }
        if (options_.skip_water)  // the skipped planting's water / fertilizer would act on an empty tile
            for (int u = 0; u < action.n_units; ++u)
                if ((action.units[u].op == OP_WATER || action.units[u].op == OP_FERTILIZE) &&
                    unplanted_[obs.self().pos_y[u] * BOARD + obs.self().pos_x[u]])
                    action.units[u] = UnitAction{};
    }
    // Inputs kept back: the peak stock later pickups need beyond the purchases usable by then,
    // then tomorrow's first feed (1 wheat per animal) beyond tonight's deposit.
    int reserve[N_PRODUCTS]{}, overnight[N_PRODUCTS]{};
    if (obs.day < LAST_DAY) {
        int animals = 0;
        for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(obs, cell).has_animal;
        overnight[WHEAT] = obs.day <= options_.feed_next ? 0 : std::max(0, animals - plan_.night_items[WHEAT]);
    }
    for (int p : {int(WHEAT), int(FERTILIZER)}) {
        int running = 0;
        for (int t = h; t < plan_.hours; ++t) {
            for (int u = 0; u < plan_.actions[t].n_units; ++u)
                if (plan_.actions[t].units[u].op == OP_PICKUP && plan_.actions[t].units[u].arg == p) running += plan_.actions[t].units[u].n;
            reserve[p] = std::max(reserve[p], running);
            for (int k = 0; k < plan_.actions[t].n_orders; ++k)
                if (plan_.actions[t].orders[k].op == M_BUY_PRODUCT && plan_.actions[t].orders[k].item == p) running -= plan_.actions[t].orders[k].n;
        }
        reserve[p] = std::max(reserve[p], running + overnight[p]);
    }
    // Room left in the shed after the last market for tonight's deposit.
    int future_pickups = 0, future_purchases = 0, sellable = 0;
    for (int later = h + 1; later < plan_.hours; ++later)
        for (int u = 0; u < plan_.actions[later].n_units; ++u)
            if (plan_.actions[later].units[u].op == OP_PICKUP) future_pickups += plan_.actions[later].units[u].n;
    for (int later = h; later < plan_.hours; ++later)
        for (int k = 0; k < plan_.actions[later].n_orders; ++k) {
            const auto& o = plan_.actions[later].orders[k];
            if (o.op == M_BUY_ANIMAL || o.op == M_BUY_PRODUCT) future_purchases += o.n;
        }
    for (int p = 0; p < N_PRODUCTS; ++p) sellable += std::max(0, obs.own.shed[p] - reserve[p]);
    const int fixed_end = obs.own.shed_total - sellable + future_purchases - future_pickups;
    const int night_room = std::max(0, 100 - plan_.night_carried - fixed_end - 2);
    static const bool room_log = std::getenv("DC12_ROOMLOG") != nullptr;
    if (room_log && h == plan_.hours - 1) {  // roomlog,day,hour,planned_carried,pockets,night_room,shed_total,sellable,fixed_end
        int pockets = 0;
        for (int u = 0; u < obs.self().n_units; ++u)
            for (int i = 0; i < N_ITEMS; ++i) pockets += obs.own.inv[u][i];
        std::fprintf(stderr, "roomlog,%d,%d,%d,%d,%d,%d,%d,%d\n", obs.day, h, plan_.night_carried, pockets, night_room, int(obs.own.shed_total), sellable, fixed_end);
    }
    DayMarket market = market_;
    if (market.scenarios > 0) {  // the plan was funded with the deterministic seller: hedge only once today's purchases are covered
        double committed = 0;
        for (int t = h; t < plan_.hours; ++t) committed += planned_cost(obs, plan_.actions[t]);
        if (obs.self().money < committed * 1.05 + 20) market.scenarios = 0;
    }
    if (options_.market.intraday) options_.market.intraday(obs, market.rival);
    mix_oracle(options_.market, obs.day, h, market.rival);
    if (options_.market.stock_cap && !options_.market.oracle) cap_by_stock(obs, history, market.rival, h, plan_.hours);
    if (market.lumpy)
        for (int t = 0; t < HOURS; ++t)
            for (int p = 0; p < N_PRODUCTS; ++p) market.lump_q[t][p] = history.sell_frequency(obs.day, 5, t, p);
    if (market.race || market.response || market.leader) {  // dc12 seller: the last day's mean inventory and the opponent's available stock
        int visible[N_PRODUCTS];
        visible_supply(obs, visible);
        int animals = 0;
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) animals += obs.opponent().tiles[y][x].has_animal;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            market.race_level[p] = history.mean_inventory(obs.step, HOURS, p) - (market.race ? 2 : 0);  // race v1: -2; response: the mean
            market.race_opp[p] = p == FERTILIZER ? animals : history.opponent_stock()[p] + visible[p];
            market.race_stock[p] = p == FERTILIZER ? animals : history.opponent_stock()[p];
        }
        if (market.leader > 0) history.own_expected(obs.day, 3, market.own_hist);
    }
    if (options_.regime_on(obs.day)) {  // regime M: carried products are held overnight and sold dawn-first
        market.carry_wait = options_.regime_wait;
        market.carry_hold = options_.regime_hold;
        for (int p = 0; p < N_PRODUCTS; ++p) market.carry[p] = options_.regime_carry >> p & 1, market.dawn[p] = options_.regime_dawn >> p & 1;
    }
    if (options_.regime_on(obs.day) && options_.regime_pockets && obs.day < LAST_DAY - 1)  // regime M: tonight's pockets sell tomorrow too
        for (int p = 0; p < N_PRODUCTS; ++p) market.hold_supply[p] += plan_.night_items[p];
    if (options_.hold_own && obs.day < LAST_DAY - 1 && plan_.hours > 0)  // holdown: our own supply tomorrow competes with units held tonight
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT) {
                const int today = plan_.receipts[plan_.hours - 1][p] + plan_.night_items[p];  // today's production, as tomorrow's estimate
                market.hold_supply[p] += plan_.night_items[p] + today / 2;  // tonight's pockets + half a day's output by tomorrow noon
            }
    if (options_.market.next_morning && obs.day < LAST_DAY - 1) options_.market.next_morning(obs, market.rival, market.hold_supply);  // local addition
    field_flow(obs, options_.market.field_flow, market.rival);  // fieldflow (dc12)
    if (obs.day >= LAST_DAY && !options_.market.oracle)  // day 29: the opponent's stock reaches the market now
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT && p != FERTILIZER) market.rival[h % HOURS][p] += history.opponent_stock()[p];
    int sell[N_PRODUCTS]{};
    double charge = 0;
    if (h == 0 && plan_.dawn_wheat_sale > 0) reserve[WHEAT] = 0;  // wheatcash: the dawn wheat is sold now
    double need[HOURS];  // cashsell: dollars missing for the planned purchases up to each later hour
    bool short_of_cash = false;
    if (options_.cash_sell && obs.day < LAST_DAY) {
        double cost = 0;
        for (int t = h; t < plan_.hours; ++t) {
            cost += planned_cost(obs, plan_.actions[t]);
            need[t - h] = cost > 0 ? cost * 1.05 + 20 - obs.self().money : 0;
            short_of_cash |= need[t - h] > 0;
        }
    }
    // sellmodel=3 / 4: the model picks the hours (threshold decode), the DP sizes the lots (3: forced lot >= 1; 4: the DP's own lot)
    bool flag[N_PRODUCTS]{};
    const bool timing = options_.sell_model && options_.sell_model_mode >= 3 && obs.day <= options_.sell_model_last &&
                        obs.day >= options_.sell_model_first;
    if (timing) {
        int units[N_PRODUCTS];
        bcsell::sell_units(*options_.sell_model, obs, history, sold_today_, options_.sell_seed, units, true, options_.sell_tracker);
        for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) flag[p] = units[p] > 0;
    }
    market.no_fire = options_.no_fire;
    choose_sales(obs, plan_.receipts, plan_.hours, market, reserve, night_room, sell, &charge, short_of_cash ? need : nullptr,
                 timing && options_.sell_model_mode == 3 ? flag : nullptr);
    if (timing && charge == 0)
        for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)})
            if (!flag[p]) sell[p] = 0;
    static const bool cash_log = std::getenv("DC11_CASHLOG") != nullptr;  // local (Imitation): units sold only to cover a cash shortfall
    if (cash_log && short_of_cash && !g_sell_quiet) {  // live executor only (not the compiler's executor simulations)
        int plain[N_PRODUCTS]{};
        choose_sales(obs, plan_.receipts, plan_.hours, market, reserve, night_room, plain, nullptr, nullptr);
        std::fprintf(stderr, "cashsell p%d d%d h%d money %.0f need %.0f |", obs.player, obs.day, h, obs.self().money, need[0]);
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (sell[p] != plain[p]) std::fprintf(stderr, " %d:%d>%d@%d", p, plain[p], sell[p], int(market_price(p, obs.market.inventory[p])));
        std::fprintf(stderr, "\n");
    }
    if (h == 0) sell[WHEAT] = std::max(sell[WHEAT], std::min<int>(plan_.dawn_wheat_sale, obs.own.shed[WHEAT]));
    if (options_.band_sell && obs.day < LAST_DAY && h + 1 < plan_.hours)  // bandsell: price-responsive rule seller
        for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) {
            const Options::BandCurve& c = options_.band[p];
            if (c.r < 0) continue;
            const double off = double(obs.market.inventory[p] - MARKET[p].I0);
            const double rate = off <= c.a ? c.r : off >= c.b ? c.s : c.r + (c.s - c.r) * (off - c.a) / (c.b - c.a);
            const int avail = std::max(0, int(obs.own.shed[p]) - reserve[p]);
            band_acc_[p] = std::min(band_acc_[p] + rate * avail, double(options_.band_lot));
            const int q = std::min(int(band_acc_[p]), avail);
            band_acc_[p] -= q;
            sell[p] = options_.band_room && charge > 0 ? std::max(q, sell[p]) : q;
        }
    if (options_.resp_sell > 0)  // respsell: price-selective test-opponent seller
        for (int p : {int(STRAWBERRY), int(MILK), int(WOOL)}) {
            const double price = market_price(p, obs.market.inventory[p]);
            resp_ema_[p] = resp_ema_[p] > 0 ? resp_ema_[p] + options_.resp_alpha * (price - resp_ema_[p]) : price;
            if (obs.day >= LAST_DAY || h + 1 >= plan_.hours) continue;
            const int available = std::max(0, obs.own.shed[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0) - reserve[p]);
            const int want = options_.resp_filter ? sell[p] : available;
            const int q = price >= options_.resp_sell * resp_ema_[p] ? std::min(options_.resp_lot, want) : 0;
            sell[p] = charge > 0 ? std::max(q, sell[p]) : q;
        }
    // Tick racer (test opponents, top players' rule): milk and wool only in the turn after each
    // shop purchase (shops buy at the end of every 4th turn), up to race_lot units each.
    if (options_.race_lot > 0 && obs.day < LAST_DAY)
        for (int p : {int(MILK), int(WOOL)}) {
            const int available = obs.own.shed[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0) - reserve[p];
            sell[p] = obs.step % 4 == 1 ? std::clamp(available, 0, options_.race_lot) : 0;
        }
    const int mm = options_.sell_model_mode;
    if (options_.sell_model && (mm <= 2 || mm == 5) && obs.day <= options_.sell_model_last && obs.day >= options_.sell_model_first) {  // sellmodel (I1): the learned M&M seller's units replace the DP's lots
        int units[N_PRODUCTS];
        bcsell::sell_units(*options_.sell_model, obs, history, sold_today_, options_.sell_seed, units, mm == 2 || mm == 5, options_.sell_tracker);
        for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) sell[p] = charge > 0 && mm != 5 ? std::max(units[p], sell[p]) : units[p];
    }
    // dawnshare=P (dc12 probe, the teachers' dawn lots): on days dawnfrom..LAST_DAY-1 at least P% of the dawnmask products' dawn stock
    // sells over h0-2 (the DP's lots count toward it).
    if (options_.dawn_share > 0 && obs.day >= options_.dawn_from && obs.day < LAST_DAY && h < 3)
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (options_.dawn_mask >> p & 1) {
                const int avail = std::max(0, int(obs.own.shed[p]) - reserve[p]);
                if (h == 0) dawn_target_[p] = (avail * options_.dawn_share + 99) / 100, dawn_issued_[p] = 0;
                sell[p] = std::max(sell[p], std::min(avail, (std::max(0, dawn_target_[p] - dawn_issued_[p]) + 2 - h) / (3 - h)));
                dawn_issued_[p] += sell[p];
            }
    int dp_sell[N_PRODUCTS];  // DC12_SELLLOG probe: the seller's decision before the cover / overflow sales
    std::copy_n(sell, N_PRODUCTS, dp_sell);
    // Cover this hour's purchases: sell more of the highest-priced stock if needed.
    const double base_cost = planned_cost(obs, planned);
    const double cost = base_cost > 0 ? base_cost * 1.05 + 20 : 0;
    double cash = obs.self().money;
    int stock[N_PRODUCTS]{};
    for (int p = 0; p < N_PRODUCTS; ++p) {
        stock[p] = obs.own.shed[p];
        for (int k = 0; k < sell[p]; ++k) cash += market_price(p, obs.market.inventory[p] + k);
    }
    auto cover = [&](int* lots, double have) {  // sell the highest-priced stock until this hour's purchases are covered
        while (have < cost) {
            int best = -1;
            for (int p = 0; p < N_PRODUCTS; ++p)
                if (stock[p] - reserve[p] > lots[p] &&
                    (best < 0 || market_price(p, obs.market.inventory[p] + lots[p]) > market_price(best, obs.market.inventory[best] + lots[best])))
                    best = p;
            if (best < 0) break;
            have += market_price(best, obs.market.inventory[best] + lots[best]);
            ++lots[best];
        }
        return have;
    };
    if (options_.no_fire && cash < cost) {  // nofire: cover sales only if they cover the purchases (else the purchase waits)
        int trial[N_PRODUCTS];
        std::copy_n(sell, N_PRODUCTS, trial);
        if (const double have = cover(trial, cash); have >= cost) std::copy_n(trial, N_PRODUCTS, sell), cash = have;
    } else cash = cover(sell, cash);
    // Shed capacity: leave room for the next deposits and this hour's purchases.
    int deposits_now = 0, pickups_now = 0, deposits_next = 0, bought = 0;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        deposits_now += plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0);
        if (h + 1 < plan_.hours) deposits_next += plan_.receipts[h + 1][p] - plan_.receipts[h][p];
    }
    for (int u = 0; u < std::min(obs.self().n_units, planned.n_units); ++u)
        if (planned.units[u].op == OP_PICKUP) pickups_now += std::min<int>(planned.units[u].n, obs.own.shed[planned.units[u].arg]);
    for (int k = 0; k < planned.n_orders; ++k)
        if (planned.orders[k].op == M_BUY_ANIMAL || planned.orders[k].op == M_BUY_PRODUCT) bought += planned.orders[k].n;
    int cover_sell[N_PRODUCTS];
    std::copy_n(sell, N_PRODUCTS, cover_sell);
    int shed = obs.own.shed_total + deposits_now - pickups_now + bought;
    for (int p = 0; p < N_PRODUCTS; ++p) shed -= std::min(sell[p], stock[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0));
    while (shed + deposits_next > 100) {
        int best = -1;
        double best_loss = 1e18;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            const int available = stock[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0);
            if (available - reserve[p] <= sell[p]) continue;
            const int now = market_price(p, obs.market.inventory[p] + sell[p]);
            const double loss = options_.market.hold_discount * market_price(p, obs.market.inventory[p] - 30) - now;
            if (loss < best_loss) best_loss = loss, best = p;
        }
        if (best < 0) break;
        ++sell[best];
        --shed;
    }
    static const bool sell_log = std::getenv("DC12_SELLLOG") != nullptr;
    if (sell_log && (h <= 2 || h >= 20))  // selllog,day,hour,product,shed,inventory,dp,cover,overflow,charge,cost,money,night_room
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (sell[p] > 0)
                std::fprintf(stderr, "selllog,%d,%d,%d,%d,%d,%d,%d,%d,%.1f,%.0f,%.0f,%d\n", obs.day, h, p, int(obs.own.shed[p]),
                             int(obs.market.inventory[p]), dp_sell[p], cover_sell[p] - dp_sell[p], sell[p] - cover_sell[p], charge, cost,
                             double(obs.self().money), night_room);
    if (sell_log && h >= 20)  // nightlog,day,hour,night_room,night_carried,fixed_end,shed_total,sellable,charge
        std::fprintf(stderr, "nightlog,%d,%d,%d,%d,%d,%d,%d,%.1f\n", obs.day, h, night_room, plan_.night_carried, fixed_end, int(obs.own.shed_total), sellable, charge);
    // Cash guard: seed and animal purchases never leave less than a few dollars.
    Action guarded = planned;
    if (obs.day < LAST_DAY) {
        double cost_all = planned_cost(obs, planned);
        int kept = 0;
        for (int k = 0; k < planned.n_orders; ++k) {
            const auto& o = planned.orders[k];
            if ((o.op == M_BUY_SEED || o.op == M_BUY_ANIMAL) && cash - cost_all < 8) {
                Action one;
                one.n_orders = 1;
                one.orders[0] = o;
                cost_all -= planned_cost(obs, one);
                // cashsell: as many of the seeds as the cash covers (the plantings without a seed are skipped)
                const int n = o.op == M_BUY_SEED && options_.cash_sell ? int((cash - cost_all - 8) / CROPS[o.item].seed) : 0;
                if (n > 0) {
                    guarded.orders[kept++] = {o.op, o.item, std::min(n, o.n)};
                    cost_all += double(CROPS[o.item].seed) * std::min(n, o.n);
                }
                continue;
            }
            guarded.orders[kept++] = o;
        }
        guarded.n_orders = kept;
        if (options_.buy_ahead) {
            kept = 0;  // this hour's planned animals already bought earlier
            for (int k = 0; k < guarded.n_orders; ++k) {
                auto o = guarded.orders[k];
                if (o.op == M_BUY_ANIMAL) {
                    const int a = o.item - GOOSE, used = std::min<int>(o.n, ahead_[a]);
                    ahead_[a] -= used, consumed_[h][a] += used, o.n = uint8_t(o.n - used);
                    if (o.n <= 0) continue;
                }
                guarded.orders[kept++] = o;
            }
            guarded.n_orders = kept;
            // the first later animal not yet bought: now, if the cash left covers it and every purchase planned before it
            double left = cash - planned_cost(obs, guarded), committed = 0;
            int spare[N_ANIMALS];
            std::copy_n(ahead_, N_ANIMALS, spare);
            bool done = false;
            for (int t = h + 1; t < plan_.hours && !done; ++t)
                for (int k = 0; k < plan_.actions[t].n_orders && !done; ++k) {
                    const auto& o = plan_.actions[t].orders[k];
                    Action one;
                    one.n_orders = 1;
                    one.orders[0] = o;
                    if (o.op != M_BUY_ANIMAL) {
                        if (o.op != M_SELL) committed += planned_cost(obs, one);
                        continue;
                    }
                    const int a = o.item - GOOSE, cover = std::min<int>(o.n, spare[a]), need = o.n - cover;
                    spare[a] -= cover;
                    if (need <= 0) continue;
                    done = true;
                    const double cost = double(ANIMALS[a].cost) * need;
                    if (left - committed >= cost + 20 && guarded.n_orders < 10) {
                        guarded.orders[guarded.n_orders++] = {M_BUY_ANIMAL, o.item, uint8_t(need)};
                        ahead_[a] += need;
                    }
                }
        }
    }
    // Orders: sales by revenue at stake (earlier slots get the higher prices), then the plan's.
    int slots = 10 - guarded.n_orders;
    int order[N_PRODUCTS];
    std::iota(order, order + N_PRODUCTS, 0);
    double key[N_PRODUCTS];
    for (int p = 0; p < N_PRODUCTS; ++p)
        key[p] = double(market_price(p, obs.market.inventory[p]) - market_price(p, obs.market.inventory[p] + sell[p])) * sell[p];
    if (options_.slot_rival > 0)
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (sell[p] > 0 && market.rival[h][p] > 0) {
                const int e = int(std::lround(market.rival[h][p]));
                key[p] += options_.slot_rival * sell[p] * double(market_price(p, obs.market.inventory[p]) - market_price(p, obs.market.inventory[p] + e));
            }
    std::stable_sort(order, order + N_PRODUCTS, [&](int a, int b) { return key[a] > key[b]; });
    action.n_orders = 0;
    for (int k = 0; k < N_PRODUCTS && slots > 0; ++k)
        if (const int p = order[k]; sell[p] > 0) {
            action.orders[action.n_orders++] = {M_SELL, uint8_t(p), sell[p]};
            --slots;
        }
    for (int k = 0; k < guarded.n_orders; ++k) action.orders[action.n_orders++] = guarded.orders[k];
    for (int k = 0; k < action.n_orders; ++k)
        if (action.orders[k].op == M_SELL && action.orders[k].item < N_PRODUCTS) sold_today_[action.orders[k].item] += action.orders[k].n;
    action.finalize();
}
}

namespace dc11 {
Options options_from_env() { return parse_options(std::getenv("DC11_OPTIONS")); }

Options parse_options(const char* text) {
    Options o;
    if (!text || std::string(text) == "-") return o;  // "-": the defaults
    char key[32];
    double value = 0;
    const char* p = text;
    for (; std::sscanf(p, " %31[a-z_]=%lf", key, &value) == 2;) {
        const std::string k = key;
        if (k == "wage") o.wage_per_turn = value;
        else if (k == "hold") o.market.hold_discount = value;
        else if (k == "rival") o.market.rival_weight = value;
        else if (k == "rivalnight") o.market.rival_night = value;
        else if (k == "fieldflow") o.market.field_flow = value;
        else if (k == "ticksell") o.market.tick_share = value;
        else if (k == "tickdawn") o.market.tick_dawn = value;
        else if (k == "tickegg") o.market.tick_egg = value;
        else if (k == "first") o.market.first_full = int(value);
        else if (k == "stress") o.stress_hour = int(value);
        else if (k == "firstfund") o.first_funding = value != 0;
        else if (k == "radius") o.search_radius = int(value);
        else if (k == "rounds") o.search_rounds = int(value);
        else if (k == "dropany") o.drop_any = int(value);
        else if (k == "downprobe") o.down_probe = int(value);
        else if (k == "quad_radius") o.q4_radius = int(value);
        else if (k == "quad_rounds") o.q4_rounds = int(value);
        else if (k == "sites") o.site_candidates = int(value);
        else if (k == "hires") o.max_hires = std::min(int(value), MAX_WORKERS - 1);
        else if (k == "finalk") o.return_cost = value;
        else if (k == "blend") o.market.blend = value;
        else if (k == "tshift") o.timing_shift = int(value);
        else if (k == "tlen") o.timing_length = int(value);
        else if (k == "grid") o.market.grid_step = int(value);
        else if (k == "site") o.site_scale = value;
        else if (k == "timing") o.timing_mix = value;
        else if (k == "capcollect") o.cap_collect = value != 0;
        else if (k == "racelot") o.race_lot = int(value);
        else if (k == "interleave") o.market.interleave = value != 0;
        else if (k == "tieall") o.market.tie_all = value != 0;
        else if (k == "hourdisc") o.market.wait_cost = 1 - value;  // each hour of waiting keeps this share of the later value (Weaknesses session)
        else if (k == "hdroute") o.market.wait_route_only = value != 0;
        else if (k == "scen") o.market.scenarios = int(value);
        else if (k == "pricefloor") o.market.floor_sales = value != 0;
        else if (k == "futurefloor") o.market.future_floor = value != 0;
        else if (k == "menu") o.plan_menu = int(value);
        else if (k == "nightfix") o.night_rounds = int(value);
        else if (k == "freshhist") o.fresh_history = value != 0;
        else if (k == "final") o.final_return_always = value != 0;
        else if (k == "budget") o.max_evaluations = long(value);
        else if (k == "landtrim") o.land_trims = int(value);
        else if (k == "cropfirst") o.crop_first = int(value);
        else if (k == "trimnet") o.trim_net = value != 0;
        else if (k == "slotrival") o.slot_rival = value;
        else if (k == "cashsell") o.cash_sell = value != 0;
        else if (k == "wheatcash") o.wheat_cash = value != 0;
        else if (k == "lazyfeed") o.lazy_feed = value != 0;
        else if (k == "reserve") o.dawn_reserve = value != 0;
        else if (k == "reservek") o.reserve_slack = value;
        else if (k == "reservenet") o.reserve_net = value != 0;
        else if (k == "nofire") o.no_fire = value != 0;
        else if (k == "latehire") o.late_hire = value != 0;
        else if (k == "splitfert") o.split_fert = int(value);
        else if (k == "dawnshare") o.dawn_share = int(value);
        else if (k == "dawnmask") o.dawn_mask = int(value);
        else if (k == "dawnfrom") o.dawn_from = int(value);
        else if (k == "nearanimals") o.near_animals = int(value);
        else if (k == "cashshadow") o.cash_shadow = value != 0;
        else if (k == "hirecheck") o.hire_check = value != 0;
        else if (k == "animalfirst") o.animal_first = value == 1, o.animal_first_mode = int(value);
        else if (k == "regime") o.regime = int(value);
        else if (k == "regimefrom") o.regime_from = int(value);
        else if (k == "regimewait") o.regime_wait = value;
        else if (k == "regimeroom") o.regime_room = int(value);
        else if (k == "regimelimit") o.regime_limit = int(value);
        else if (k == "regimecarry") o.regime_carry = int(value);
        else if (k == "regimeeve") o.regime_eve = int(value);
        else if (k == "regimehold") o.regime_hold = value;
        else if (k == "regimepockets") o.regime_pockets = value != 0;
        else if (k == "regimemorning") o.regime_morning = value;
        else if (k == "regimedawn") o.regime_dawn = int(value);
        else if (k == "collectmin") o.collect_min = value;
        else if (k == "startcomplete") o.day0_complete = int(value);
        else if (k == "feednext") o.feed_next = int(value);
        else if (k == "prodmin") o.product_min = value;
        else if (k == "feedlate") o.feed_late = int(value);
        else if (k == "defercollect") o.defer_collect = value != 0;
        else if (k == "optlate") o.optional_late = value != 0;
        else if (k == "lotcap") o.market.lot_cap = int(value);
        else if (k == "race") o.market.race = value != 0;
        else if (k == "leader") o.market.leader = value;
        else if (k == "affordanimal") o.afford_animal = value != 0;
        else if (k == "buyahead") o.buy_ahead = value != 0;
        else if (k == "saleslots") o.sale_slots = int(value);
        else if (k == "cashroute") o.cash_route = value;
        else if (k == "depcredit") o.dep_credit = value;
        else if (k == "dephour") o.dep_hour = int(value);
        else if (k == "depfert") o.dep_fert = value != 0;
        else if (k == "cashseeds") o.cash_seeds = value != 0;
        else if (k == "skipwater") o.skip_water = value != 0;
        else if (k == "achieve") o.achieve = value != 0, o.achieve_mode = int(value), o.skip_water = o.skip_water || o.achieve;
        else if (k == "seedlag") o.seed_lag = value != 0;
        else if (k == "feedcost") o.feed_cost = value;
        else if (k == "sellmodel") o.sell_model_on = value != 0, o.sell_model_mode = int(value);
        else if (k == "sellmodellast") o.sell_model_last = int(value);
        else if (k == "sellmodelfirst") o.sell_model_first = int(value);
        else if (k == "keepfed") o.keep_fed = value != 0;
        else if (k == "nighttrim") o.night_trim = value != 0;
        else if (k == "affordall") o.afford_all = value != 0;
        else if (k == "bestvariant") o.best_variant = value != 0;
        else if (k == "holdown") o.hold_own = value != 0;
        else if (k == "holdsteps") o.market.hold_steps = int(value);
        else if (k == "landfirst") o.land_first = value != 0;
        else if (k == "landpull") o.land_pull = int(value);
        else if (k == "survivalfloor") o.survival_floor = value != 0;
        else if (k == "response") o.market.response = value != 0;
        else if (k == "stockcap") o.market.stock_cap = value != 0;
        else if (k == "scenopen") o.market.scen_open = value != 0;
        else if (k == "lumpy") o.market.lumpy = value != 0;
        else if (k == "bandsell") o.band_sell = value != 0;
        else if (k == "respsell") o.resp_sell = value;
        else if (k == "resplot") o.resp_lot = int(value);
        else if (k == "respalpha") o.resp_alpha = value;
        else if (k == "respfilter") o.resp_filter = value != 0;
        else if (k == "bandlot") o.band_lot = int(value);
        else if (k == "bandroom") o.band_room = value != 0;
        else if (k.size() >= 4 && std::string("abrs").find(k.back()) != std::string::npos &&
                 (k.substr(0, k.size() - 1) == "straw" || k.substr(0, k.size() - 1) == "egg" || k.substr(0, k.size() - 1) == "milk" ||
                  k.substr(0, k.size() - 1) == "wool")) {
            const std::string name = k.substr(0, k.size() - 1);
            const int p = name == "straw" ? STRAWBERRY : name == "egg" ? EGG : name == "milk" ? MILK : WOOL;
            double& field = k.back() == 'a' ? o.band[p].a : k.back() == 'b' ? o.band[p].b : k.back() == 'r' ? o.band[p].r : o.band[p].s;
            field = value;
        }
        else if (k == "dawnsell") o.market.dawn_sell = int(value);
        else if (k == "dawnstart") o.market.dawn_start = int(value);
        else std::abort();
        while (*p == ' ') ++p;
        while (*p && *p != ' ') ++p;
    }
    while (*p == ' ') ++p;
    if (*p) std::abort();  // text that is not key=value: never play the defaults silently
    return o;
}
}
