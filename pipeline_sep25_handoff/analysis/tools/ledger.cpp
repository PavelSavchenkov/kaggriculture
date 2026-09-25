// Full-game ledger: replays traces in the exact engine and records, per seat, where
// money came from and went, what each crop cohort and animal produced, and every loss
// (decay, weeds, escapes, held-cap clipping, lost care bonus, shed discards, unsold
// stock at the end).
// Exact attribution: sanitize_joint_actions gives the successful unit actions and the
// committed order quantities; per-unit market prices are replayed in the engine's
// lockstep order and checked against the engine's revenue and spend counters.
// usage: ledger list.txt out_prefix   (list line: trace label0 label1 [group])
// Writes <prefix>_games.csv, _days.csv, _crops.csv, _animals.csv.
#include "source/world.hpp"
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>

using namespace dc10;

namespace {
constexpr int NP = N_PRODUCTS;
const char* PN[NP] = {"wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"};
const char* AN[N_ANIMALS] = {"goose", "cow", "sheep"};

// Action categories for worker time.
enum Cat { MOVE, PLANT, WATER, HARVEST_CROP, HARVEST_ANIMAL, FERTILIZE, DIG, BUILD, PLACE_ANIMAL, SHED_IO, FEED, CARE,
           COLLECT_FERT, IDLE, NCAT };
const char* CN[NCAT] = {"move", "plant", "water", "harvest_crop", "harvest_animal", "fertilize", "dig", "build",
                        "place_animal", "shed_io", "feed", "care", "collect_fert", "idle"};

struct Day {
    double money_dawn = 0, money_end = 0;
    int shed_dawn = 0, shed_prod_dawn[NP]{}, land = 0, plants[N_CROPS]{}, animals[N_ANIMALS]{}, empty_owned = 0,
        weeds_owned = 0, empty_struct = 0, owned = 0;
    double stock_value_dawn = 0;  // sellable shed stock (not wheat/fertilizer) at dawn prices
    int workers = 1, hires = 0;
    int u_turns[MAX_UNITS]{}, u_work[MAX_UNITS]{}, u_move[MAX_UNITS]{}, hire_hour[MAX_UNITS]{};  // per unit (LEDGER_HIRES)
    int h_work[HOURS]{}, h_move[HOURS]{}, h_idle[HOURS]{}, h_units[HOURS]{}, h_shed[HOURS]{};  // per hour (LEDGER_HIRES)
    double hire_cost = 0, land_cost = 0, revenue = 0, rev[NP]{}, seed_cost[N_CROPS]{}, animal_cost[N_ANIMALS]{},
           buy_cost[NP]{};
    int sold[NP]{}, bought[NP]{}, seeds_bought[N_CROPS]{}, animals_bought[N_ANIMALS]{}, planted[N_CROPS]{},
        placed[N_ANIMALS]{}, harvested[NP]{}, discard_day[N_ITEMS]{}, discard_night[N_ITEMS]{};
    int cat[NCAT]{}, unit_turns = 0;
    int sold_hour[4]{};  // hours 0-5, 6-11, 12-17, 18-23
    double mean_price[NP]{}, max_price[NP]{}, min_price[NP]{};
    int decay_lost[N_CROPS]{}, weed_dry[N_CROPS]{}, weed_dry_yield[N_CROPS]{}, weed_decay[N_CROPS]{}, cap_lost_crop = 0;
    int seeds_dawn[N_CROPS]{}, animal_dist = 0, animal_n = 0, crop_dist = 0, crop_n = 0, placed_dist = 0, planted_dist = 0;
    int escapes[N_ANIMALS]{}, cap_lost_animal[N_ANIMALS]{}, bonus_lost = 0, unfed = 0, fed = 0, cared = 0,
        fert_missed = 0, night_carried = 0, shed_night = 0;
};

struct CropInst {
    int crop = 0, day = 0, cell = 0;
    int harvested = 0, harvests = 0, waters = 0, ferts = 0, decay_lost = 0, cap_lost = 0;
    int fate = -1, end_day = -1, lost_at_end = 0;  // fate: 0 harvested away, 1 dug, 2 weed dry, 3 weed decay, 4 in ground at end
};
const char* FATE[5] = {"harvested", "dug", "weed_dry", "weed_decay", "end"};

struct AnimalInst {
    int species = 0, day = 0, cell = 0;
    int fed = 0, cared = 0, unfed = 0, collected = 0, collections = 0, cap_lost = 0, bonus_lost = 0, fert = 0,
        fert_missed = 0, escaped = -1, prod_units = 0, held_end = 0;
};

struct Seat {
    std::string label;
    Day days[30];
    std::vector<CropInst> crops;
    std::vector<AnimalInst> animals;
    std::map<int, int> crop_at, animal_at;  // cell -> live instance index
    double final_money = 0, unsold_value = 0, carried_value = 0, held_value = 0;
    int unsold_units = 0, carried_units = 0, held_units = 0;
};

bool same_plant(const Tile& a, const Tile& b) {
    return a.kind == T_PLANT && b.kind == T_PLANT && a.what == b.what && a.planted_day == b.planted_day;
}
bool same_animal(const Tile& a, const Tile& b) {
    return a.has_animal && b.has_animal && a.what == b.what && a.planted_day == b.planted_day;
}

void snapshot_dawn(const Sim& sim, int seat, Day& d) {
    const Farm& f = sim.st.farms[seat];
    d.money_dawn = f.money;
    d.shed_dawn = f.shed_total;
    for (int p = 0; p < NP; ++p) {
        d.shed_prod_dawn[p] = f.shed[p];
        if (p != WHEAT && p != FERTILIZER) d.stock_value_dawn += double(f.shed[p]) * sim.st.market.prices[p];
    }
    d.land = f.n_quadrants;
    for (int c = 0; c < N_CROPS; ++c) d.seeds_dawn[c] = f.seeds[c];
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = f.tiles[y][x];
            if (t.kind == T_LOCKED) continue;
            ++d.owned;
            if (t.kind == T_PLANT) ++d.plants[t.what], d.crop_dist += shed_dist(cell_of(x, y)), ++d.crop_n;
            if (t.has_animal) ++d.animals[t.what - GOOSE], d.animal_dist += shed_dist(cell_of(x, y)), ++d.animal_n;
            if (t.kind == T_EMPTY) ++d.empty_owned;
            if (t.kind == T_WEED) ++d.weeds_owned;
            if ((t.kind == T_COOP || t.kind == T_PASTURE) && !t.has_animal) ++d.empty_struct;
        }
}

// Exact per-unit prices of both players' committed orders, in the engine's lockstep.
std::ofstream* sales_out = nullptr;  // optional <prefix>_sales.csv (LEDGER_SALES=1)
std::string current_trace;
std::ofstream* units_out = nullptr;  // optional <prefix>_units.csv (LEDGER_HIRES=1)
std::ofstream* hours_out = nullptr;  // optional <prefix>_hours.csv (LEDGER_HIRES=1)

void replay_market(const Sim& before, const std::array<Action, 2>& san, int day, Seat* seats[2], int hour) {
    int units[2][NP]{};
    double paid[2][NP]{};
    int inventory[NP];
    std::copy_n(before.st.market.inventory, NP, inventory);
    int hires[2] = {before.st.farms[0].hires_today, before.st.farms[1].hires_today};
    int quadrants[2] = {before.st.farms[0].n_quadrants, before.st.farms[1].n_quadrants};
    const int max_orders = std::max(san[0].n_orders, san[1].n_orders);
    for (int k = 0; k < max_orders; ++k) {
        Order o[2];
        for (int p = 0; p < 2; ++p)
            if (k < san[p].n_orders) o[p] = san[p].orders[k];
        for (int p = 0; p < 2; ++p) {
            Day& d = seats[p]->days[day];
            if (o[p].op == M_HIRE && o[p].n > 0) {
                if (hires[p] < MAX_UNITS) d.hire_hour[hires[p]] = hour;
                d.hire_cost += fib(hires[p]++) * before.cfg.hire_mult;
                ++d.hires;
                o[p].n = 0;
            } else if (o[p].op == M_BUY_LAND && o[p].n > 0) {
                d.land_cost += LAND_PRICES[quadrants[p]++ - 1];
                o[p].n = 0;
            }
        }
        for (;;) {
            int price[2] = {-1, -1};
            for (int p = 0; p < 2; ++p) {
                if (o[p].n <= 0) continue;
                if (o[p].op == M_SELL) price[p] = market_price(o[p].item, inventory[o[p].item]);
                else if (o[p].op == M_BUY_PRODUCT) price[p] = market_price(o[p].item, inventory[o[p].item] - 1);
                else if (o[p].op == M_BUY_SEED) price[p] = CROPS[o[p].item].seed;
                else if (o[p].op == M_BUY_ANIMAL) price[p] = ANIMALS[o[p].item - GOOSE].cost;
            }
            if (price[0] < 0 && price[1] < 0) break;
            for (int p = 0; p < 2; ++p) {
                if (price[p] < 0) continue;
                Day& d = seats[p]->days[day];
                const int item = o[p].item;
                if (o[p].op == M_SELL) {
                    ++units[p][item], paid[p][item] += price[p];
                    d.revenue += price[p], d.rev[item] += price[p], ++d.sold[item];
                    ++d.sold_hour[hour / 6];
                    if (price[p] > 1) ++inventory[item];
                } else if (o[p].op == M_BUY_PRODUCT) {
                    d.buy_cost[item] += price[p], ++d.bought[item];
                    --inventory[item];
                } else if (o[p].op == M_BUY_SEED) {
                    d.seed_cost[item] += price[p], ++d.seeds_bought[item];
                } else {
                    d.animal_cost[item - GOOSE] += price[p], ++d.animals_bought[item - GOOSE];
                }
                --o[p].n;
            }
        }
    }
    if (sales_out)
        for (int p = 0; p < 2; ++p)
            for (int q = 0; q < NP; ++q)
                if (units[p][q])
                    *sales_out << current_trace << ',' << p << ',' << day << ',' << hour << ',' << q << ',' << units[p][q] << ','
                               << paid[p][q] << ',' << before.st.market.inventory[q] << '\n';
}

double spend_of(const Day& d) {
    double s = d.hire_cost + d.land_cost;
    for (int c = 0; c < N_CROPS; ++c) s += d.seed_cost[c];
    for (int a = 0; a < N_ANIMALS; ++a) s += d.animal_cost[a];
    for (int p = 0; p < NP; ++p) s += d.buy_cost[p];
    return s;
}

void process(const std::string& trace, const std::string labels[2], const std::string& group, std::ofstream& games,
             std::ofstream& days, std::ofstream& crops, std::ofstream& animals) {
    const Replay replay = load_replay(trace);
    current_trace = trace;
    Sim sim(replay.config);
    Seat seat_data[2];
    Seat* seats[2] = {&seat_data[0], &seat_data[1]};
    for (int p = 0; p < 2; ++p) seats[p]->label = labels[p];
    double price_sum[NP]{};
    int price_n = 0;
    double spent_check[2]{}, revenue_check[2]{};
    for (size_t s = 0; s < replay.turns.size(); ++s) {
        const int day = sim.st.day, hour = sim.st.hour;
        const Action& a0 = replay.turns[s][0];
        const Action& a1 = replay.turns[s][1];
        if (hour == 0) {
            for (int p = 0; p < 2; ++p) snapshot_dawn(sim, p, seats[p]->days[day]);
            std::fill_n(price_sum, NP, 0.0);
            price_n = 0;
            for (int p = 0; p < 2; ++p)
                for (int q = 0; q < NP; ++q) seats[p]->days[day].min_price[q] = 1e9;
        }
        for (int q = 0; q < NP; ++q) {
            price_sum[q] += sim.st.market.prices[q];
            for (int p = 0; p < 2; ++p) {
                seats[p]->days[day].max_price[q] = std::max(seats[p]->days[day].max_price[q], double(sim.st.market.prices[q]));
                seats[p]->days[day].min_price[q] = std::min(seats[p]->days[day].min_price[q], double(sim.st.market.prices[q]));
            }
        }
        ++price_n;
        const auto san = sim.sanitize_joint_actions(a0, a1);
        // State after unit actions and market, before decay (lifespans disabled) and night.
        Sim units_only = sim;
        units_only.cfg.turns_per_day = 1 << 20;
        for (auto& f : units_only.st.farms)
            for (auto& row : f.tiles)
                for (auto& t : row) t.max_lifespan_step = -1;
        units_only.step(a0, a1);
        const Sim mid = step_without_night(sim, a0, a1);  // after decay, before the night update
        const bool night = hour == HOURS - 1;
        const Sim before = sim;
        double spend0[2], rev0[2];
        for (int p = 0; p < 2; ++p) spend0[p] = sim.st.farms[p].total_spend, rev0[p] = sim.st.farms[p].sell_revenue;
        int prod0[2][N_ITEMS], disc0[2][N_ITEMS];
        for (int p = 0; p < 2; ++p)
            for (int i = 0; i < N_ITEMS; ++i) prod0[p][i] = sim.st.farms[p].produced[i], disc0[p][i] = sim.st.farms[p].discarded[i];
        replay_market(before, san, day, seats, hour);
        sim.step(a0, a1);
        for (int p = 0; p < 2; ++p) {
            Seat& S = *seats[p];
            Day& d = S.days[day];
            const Farm& fb = before.st.farms[p];
            const Farm& fu = units_only.st.farms[p];
            const Farm& fm = mid.st.farms[p];
            const Farm& fa = sim.st.farms[p];
            spent_check[p] += fa.total_spend - spend0[p];
            revenue_check[p] += fa.sell_revenue - rev0[p];
            for (int i = 0; i < NP; ++i) d.harvested[i] += fa.produced[i] - prod0[p][i];
            // Worker actions (successful ones only; everything else is idle).
            d.workers = std::max(d.workers, fb.n_units);
            d.unit_turns += fb.n_units;
            bool watered[BOARD * BOARD]{};
            for (int u = 0; u < fb.n_units; ++u) {
                const UnitAction& ua = san[p].units[u];
                const int cell = cell_of(fb.pos_x[u], fb.pos_y[u]);
                const Tile& t = fb.tiles[fb.pos_y[u]][fb.pos_x[u]];
                int c = IDLE;
                switch (ua.op) {
                    case OP_NORTH: case OP_SOUTH: case OP_EAST: case OP_WEST: c = MOVE; break;
                    case OP_PLANT: c = PLANT; ++d.planted[ua.arg]; d.planted_dist += shed_dist(cell); break;
                    case OP_WATER: c = WATER; watered[cell] = true; break;
                    case OP_HARVEST: c = t.has_animal ? HARVEST_ANIMAL : HARVEST_CROP; break;
                    case OP_FERTILIZE: c = FERTILIZE; break;
                    case OP_DIG: c = DIG; break;
                    case OP_BUILD_COOP: case OP_BUILD_PASTURE: c = BUILD; break;
                    case OP_PLACE: c = is_animal(ua.arg) && !shed_access(cell) ? PLACE_ANIMAL : SHED_IO;
                        if (is_animal(ua.arg) && (t.kind == T_COOP || t.kind == T_PASTURE)) c = PLACE_ANIMAL;
                        break;
                    case OP_PICKUP: case OP_DROP: c = SHED_IO; break;
                    case OP_FEED: c = FEED; break;
                    case OP_CARE: c = CARE; break;
                    case OP_COLLECT_FERTILIZER: c = COLLECT_FERT; break;
                    default: c = IDLE;
                }
                ++d.cat[c];
                ++d.u_turns[u];
                ++d.h_units[hour];
                if (c == MOVE) ++d.u_move[u], ++d.h_move[hour];
                else if (c == IDLE) ++d.h_idle[hour];
                else ++d.u_work[u], ++d.h_work[hour];
                if (c == SHED_IO) ++d.h_shed[hour];
                if (c == PLACE_ANIMAL) ++d.placed[ua.arg - GOOSE], d.placed_dist += shed_dist(cell);
                // Instance bookkeeping on the acted tile.
                if (c == WATER || c == FERTILIZE || c == HARVEST_CROP) {
                    auto it = S.crop_at.find(cell);
                    if (it != S.crop_at.end()) {
                        CropInst& ci = S.crops[it->second];
                        if (c == WATER) ++ci.waters;
                        if (c == FERTILIZE) ++ci.ferts;
                        if (c == HARVEST_CROP) {
                            int units = t.yield_units;
                            if (watered[cell] && !CROPS[t.what].ongoing) {
                                const int age = day - t.planted_day, w0 = (CROPS[t.what].max_yield_day + 1) / 2;
                                if (age >= w0 && age <= CROPS[t.what].max_yield_day)
                                    units = std::min(CROPS[t.what].max_yield, units + (t.fertilized_until_day >= day ? 2 : 1));
                            }
                            ci.harvested += units, ++ci.harvests;
                        }
                    }
                }
                if (c == HARVEST_ANIMAL || c == COLLECT_FERT) {
                    auto it = S.animal_at.find(cell);
                    if (it != S.animal_at.end()) {
                        AnimalInst& ai = S.animals[it->second];
                        if (c == HARVEST_ANIMAL) ai.collected += t.yield_units, ++ai.collections;
                        else ++ai.fert;
                    }
                }
            }
            // Tile transitions during the day: new plants/animals, removals, decay.
            for (int cell = 0; cell < BOARD * BOARD; ++cell) {
                const int x = cell_x(cell), y = cell_y(cell);
                const Tile &tb = fb.tiles[y][x], &tu = fu.tiles[y][x], &tm = fm.tiles[y][x];
                // Plant removed by unit actions (harvest of one-shot or dig).
                if (tb.kind == T_PLANT && !same_plant(tb, tu)) {
                    auto it = S.crop_at.find(cell);
                    if (it != S.crop_at.end()) {
                        CropInst& ci = S.crops[it->second];
                        const bool harvested = ci.harvests > 0 && !CROPS[ci.crop].ongoing ;
                        ci.fate = harvested ? 0 : 1;
                        ci.end_day = day;
                        if (!harvested) ci.lost_at_end = tb.yield_units;
                        S.crop_at.erase(it);
                    }
                }
                // New plant this step.
                if (tu.kind == T_PLANT && !same_plant(tb, tu)) {
                    CropInst ci;
                    ci.crop = tu.what, ci.day = day, ci.cell = cell;
                    S.crop_at[cell] = int(S.crops.size());
                    S.crops.push_back(ci);
                }
                // Decay: plant present after units; yield lower (or a weed) after the decay phase.
                if (tu.kind == T_PLANT) {
                    auto it = S.crop_at.find(cell);
                    if (it != S.crop_at.end()) {
                        CropInst& ci = S.crops[it->second];
                        if (same_plant(tu, tm)) {
                            const int lost = tu.yield_units - tm.yield_units;
                            if (lost > 0) ci.decay_lost += lost, d.decay_lost[ci.crop] += lost;
                        } else if (tm.kind == T_WEED) {
                            const int lost = std::max(0, int(tu.yield_units));
                            ci.decay_lost += lost, d.decay_lost[ci.crop] += lost;
                            ci.fate = 3, ci.end_day = day;
                            ++d.weed_decay[ci.crop];
                            S.crop_at.erase(it);
                        }
                    }
                }
                // New animal placed.
                if (tu.has_animal && !same_animal(tb, tu)) {
                    AnimalInst ai;
                    ai.species = tu.what - GOOSE, ai.day = day, ai.cell = cell;
                    S.animal_at[cell] = int(S.animals.size());
                    S.animals.push_back(ai);
                }
            }
            for (int i = 0; i < N_ITEMS; ++i) d.discard_day[i] += fm.discarded[i] - disc0[p][i];
            if (!night) continue;
            // Night update: mid (pre-night) -> sim (next dawn).
            for (int u = 0; u < fm.n_units; ++u)
                for (int i = 0; i < NP; ++i) d.night_carried += fm.inv[u][i];
            for (int i = 0; i < N_ITEMS; ++i) d.discard_night[i] += fa.discarded[i] - fm.discarded[i];
            d.shed_night = fa.shed_total;
            d.money_end = fa.money;
            const int next_day = day + 1;
            for (int cell = 0; cell < BOARD * BOARD; ++cell) {
                const int x = cell_x(cell), y = cell_y(cell);
                const Tile &tm = fm.tiles[y][x], &ta = fa.tiles[y][x];
                if (tm.kind == T_PLANT) {
                    auto it = S.crop_at.find(cell);
                    if (it == S.crop_at.end()) continue;
                    CropInst& ci = S.crops[it->second];
                    const CropDef& cd = CROPS[tm.what];
                    if (!same_plant(tm, ta)) {  // died dry
                        ci.fate = 2, ci.end_day = day, ci.lost_at_end = tm.yield_units;
                        ++d.weed_dry[ci.crop];
                        d.weed_dry_yield[ci.crop] += tm.yield_units;
                        S.crop_at.erase(it);
                        continue;
                    }
                    if (cd.ongoing) {
                        const int since = next_day - tm.planted_day - cd.first_yield_day;
                        if (since >= 0 && since % cd.interval == 0 && since / cd.interval + 1 <= cd.max_yield) {
                            const int expected = (tm.watered_today && tm.fertilized_until_day >= day) ? 2 : 1;
                            const int lost = expected - (ta.yield_units - tm.yield_units);
                            if (lost > 0) ci.cap_lost += lost, d.cap_lost_crop += lost;
                        }
                    }
                }
                if (tm.has_animal) {
                    auto it = S.animal_at.find(cell);
                    if (it == S.animal_at.end()) continue;
                    AnimalInst& ai = S.animals[it->second];
                    const AnimalDef& ad = ANIMALS[tm.what - GOOSE];
                    ai.fed += tm.fed_today, ai.cared += tm.cared_today && tm.fed_today, ai.unfed += !tm.fed_today;
                    d.fed += tm.fed_today, d.cared += tm.cared_today && tm.fed_today, d.unfed += !tm.fed_today;
                    if (tm.fertilizer_available) ++ai.fert_missed, ++d.fert_missed;
                    if (!same_animal(tm, ta)) {
                        ai.escaped = day;
                        ++d.escapes[ai.species];
                        S.animal_at.erase(it);
                        continue;
                    }
                    const int since = next_day - tm.planted_day - ad.first_yield_day;
                    if (since >= 0 && since % ad.interval == 0) {
                        const int expected = 1 + (tm.fed_today ? tm.pending_care_bonus : 0);
                        const int got = ta.yield_units - tm.yield_units;
                        ai.prod_units += got;
                        if (expected > got) ai.cap_lost += expected - got, d.cap_lost_animal[ai.species] += expected - got;
                        if (!tm.fed_today && tm.pending_care_bonus > 0) ai.bonus_lost += tm.pending_care_bonus, d.bonus_lost += tm.pending_care_bonus;
                    }
                }
            }
        }
        if (hour == HOURS - 1 || s + 1 == replay.turns.size())
            for (int p = 0; p < 2; ++p)
                for (int q = 0; q < NP; ++q) seats[p]->days[day].mean_price[q] = price_sum[q] / price_n;
    }
    // Checks against the engine counters.
    for (int p = 0; p < 2; ++p) {
        double spend = 0, revenue = 0;
        for (const Day& d : seats[p]->days) spend += spend_of(d), revenue += d.revenue;
        if (std::abs(spend - sim.st.farms[p].total_spend) > 0.5 || std::abs(revenue - sim.st.farms[p].sell_revenue) > 0.5)
            std::fprintf(stderr, "ledger mismatch %s seat %d: spend %.0f vs %.0f revenue %.0f vs %.0f\n", trace.c_str(), p, spend,
                         sim.st.farms[p].total_spend, revenue, sim.st.farms[p].sell_revenue);
    }
    // End of game: what was left.
    const int last = sim.st.day > LAST_DAY ? LAST_DAY : sim.st.day;
    for (int p = 0; p < 2; ++p) {
        Seat& S = *seats[p];
        const Farm& f = sim.st.farms[p];
        S.final_money = f.money;
        S.days[last].money_end = f.money;
        for (int i = 0; i < NP; ++i) {
            S.unsold_units += f.shed[i];
            S.unsold_value += double(f.shed[i]) * sim.st.market.prices[i];
            for (int u = 0; u < f.n_units; ++u) S.carried_units += f.inv[u][i], S.carried_value += double(f.inv[u][i]) * sim.st.market.prices[i];
        }
        for (auto& [cell, index] : S.crop_at) {
            CropInst& ci = S.crops[index];
            const Tile& t = f.tiles[cell_y(cell)][cell_x(cell)];
            ci.fate = 4, ci.end_day = LAST_DAY, ci.lost_at_end = t.yield_units;
            if (t.kind == T_PLANT && sim.st.day - t.planted_day >= CROPS[t.what].first_yield_day)
                S.held_units += t.yield_units, S.held_value += double(t.yield_units) * sim.st.market.prices[t.what];
        }
        for (auto& [cell, index] : S.animal_at) {
            AnimalInst& ai = S.animals[index];
            const Tile& t = f.tiles[cell_y(cell)][cell_x(cell)];
            ai.held_end = t.yield_units;
            S.held_units += t.yield_units;
            S.held_value += double(t.yield_units) * sim.st.market.prices[ANIMALS[ai.species].product];
        }
    }
    // Output.
    for (int p = 0; p < 2; ++p) {
        const Seat& S = *seats[p];
        const Seat& O = *seats[1 - p];
        games << trace << ',' << p << ',' << S.label << ',' << O.label << ',' << group << ',' << S.final_money << ','
              << O.final_money << ',' << S.unsold_units << ',' << S.unsold_value << ',' << S.carried_units << ','
              << S.carried_value << ',' << S.held_units << ',' << S.held_value << '\n';
        for (int dd = 0; dd <= last; ++dd) {
            const Day& d = S.days[dd];
            days << trace << ',' << p << ',' << S.label << ',' << O.label << ',' << group << ',' << dd << ',' << d.money_dawn
                 << ',' << d.money_end << ',' << d.shed_dawn << ',' << d.stock_value_dawn << ',' << d.land << ',' << d.owned
                 << ',' << d.empty_owned << ',' << d.weeds_owned << ',' << d.empty_struct << ',' << d.workers << ',' << d.hires
                 << ',' << d.hire_cost << ',' << d.land_cost << ',' << d.revenue << ',' << d.unit_turns << ','
                 << d.night_carried << ',' << d.shed_night << ',' << d.cap_lost_crop << ',' << d.bonus_lost << ',' << d.fed
                 << ',' << d.cared << ',' << d.unfed << ',' << d.fert_missed;
            for (int c = 0; c < N_CROPS; ++c)
                days << ',' << d.plants[c] << ',' << d.planted[c] << ',' << d.seeds_bought[c] << ',' << d.seed_cost[c] << ','
                     << d.decay_lost[c] << ',' << d.weed_dry[c] << ',' << d.weed_dry_yield[c] << ',' << d.weed_decay[c];
            for (int a = 0; a < N_ANIMALS; ++a)
                days << ',' << d.animals[a] << ',' << d.animals_bought[a] << ',' << d.placed[a] << ',' << d.escapes[a] << ','
                     << d.cap_lost_animal[a];
            for (int q = 0; q < NP; ++q)
                days << ',' << d.shed_prod_dawn[q] << ',' << d.sold[q] << ',' << d.rev[q] << ',' << d.bought[q] << ','
                     << d.buy_cost[q] << ',' << d.harvested[q] << ',' << d.discard_day[q] + d.discard_night[q] << ','
                     << d.mean_price[q] << ',' << d.max_price[q] << ',' << d.min_price[q];
            for (int c = 0; c < NCAT; ++c) days << ',' << d.cat[c];
            for (int h = 0; h < 4; ++h) days << ',' << d.sold_hour[h];
            int animal_discards = 0;
            for (int i = GOOSE; i < N_ITEMS; ++i) animal_discards += d.discard_day[i] + d.discard_night[i];
            days << ',' << animal_discards;
            for (int c = 0; c < N_CROPS; ++c) days << ',' << d.seeds_dawn[c];
            days << ',' << d.animal_dist << ',' << d.animal_n << ',' << d.crop_dist << ',' << d.crop_n << ',' << d.placed_dist << ',' << d.planted_dist << '\n';
            if (units_out)
                for (int u = 0; u < std::min(d.workers, MAX_UNITS); ++u)
                    *units_out << trace << ',' << p << ',' << S.label << ',' << dd << ',' << u << ','
                               << (u ? d.hire_hour[u - 1] : -1) << ',' << (u ? fib(u - 1) : 0) << ',' << d.u_turns[u] << ','
                               << d.u_work[u] << ',' << d.u_move[u] << '\n';
            if (hours_out)
                for (int h = 0; h < HOURS; ++h)
                    *hours_out << trace << ',' << p << ',' << S.label << ',' << dd << ',' << h << ',' << d.h_units[h] << ','
                               << d.h_work[h] << ',' << d.h_move[h] << ',' << d.h_idle[h] << ',' << d.h_shed[h] << '\n';
        }
        // Crop cohorts: crop x planting day.
        std::map<std::pair<int, int>, std::array<int, 12>> cohorts;
        for (const auto& ci : S.crops) {
            auto& c = cohorts[{ci.crop, ci.day}];
            ++c[0];
            c[1] += ci.harvested, c[2] += ci.harvests, c[3] += ci.waters, c[4] += ci.ferts, c[5] += ci.decay_lost,
                c[6] += ci.cap_lost, c[7 + std::max(0, ci.fate)] += 1;
        }
        for (const auto& [key, c] : cohorts) {
            crops << trace << ',' << p << ',' << S.label << ',' << group << ',' << PN[key.first] << ',' << key.second;
            for (int v : c) crops << ',' << v;
            crops << '\n';
        }
        for (const auto& ai : S.animals)
            animals << trace << ',' << p << ',' << S.label << ',' << group << ',' << AN[ai.species] << ',' << ai.day << ','
                    << ai.escaped << ',' << ai.fed << ',' << ai.cared << ',' << ai.unfed << ',' << ai.collected << ','
                    << ai.collections << ',' << ai.prod_units << ',' << ai.cap_lost << ',' << ai.bonus_lost << ',' << ai.fert
                    << ',' << ai.fert_missed << ',' << ai.held_end << '\n';
    }
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ledger list.txt out_prefix\n";
        return 2;
    }
    const std::string prefix = argv[2];
    std::ofstream games(prefix + "_games.csv"), days(prefix + "_days.csv"), crops(prefix + "_crops.csv"),
        animals(prefix + "_animals.csv");
    games << "trace,seat,label,opp_label,group,money,opp_money,unsold_units,unsold_value,carried_units,carried_value,"
             "held_units,held_value\n";
    days << "trace,seat,label,opp_label,group,day,money_dawn,money_end,shed_dawn,stock_value_dawn,land,owned,empty_owned,"
            "weeds_owned,empty_struct,workers,hires,hire_cost,land_cost,revenue,unit_turns,night_carried,shed_night,"
            "cap_lost_crop,bonus_lost,fed,cared,unfed,fert_missed";
    for (int c = 0; c < N_CROPS; ++c)
        for (const char* f : {"plants", "planted", "seeds_bought", "seed_cost", "decay_lost", "weed_dry", "weed_dry_yield", "weed_decay"})
            days << ',' << f << '_' << PN[c];
    for (int a = 0; a < N_ANIMALS; ++a)
        for (const char* f : {"animals", "bought", "placed", "escapes", "cap_lost"}) days << ',' << f << '_' << AN[a];
    for (int q = 0; q < NP; ++q)
        for (const char* f : {"shed", "sold", "rev", "bought", "buy_cost", "harvested", "discarded", "mean_price", "max_price", "min_price"})
            days << ',' << f << '_' << PN[q];
    for (int c = 0; c < NCAT; ++c) days << ",act_" << CN[c];
    days << ",sold_h0,sold_h6,sold_h12,sold_h18,discarded_animals";
    for (int c = 0; c < N_CROPS; ++c) days << ",seeds_" << PN[c];
    days << ",animal_dist,animal_n,crop_dist,crop_n,placed_dist,planted_dist\n";
    crops << "trace,seat,label,group,crop,day,n,harvested,harvests,waters,ferts,decay_lost,cap_lost";
    for (const char* f : FATE) crops << ",fate_" << f;
    crops << '\n';
    animals << "trace,seat,label,group,species,day,escaped,fed,cared,unfed,collected,collections,prod_units,cap_lost,"
               "bonus_lost,fert,fert_missed,held_end\n";
    std::ofstream sales;
    std::ofstream units_file, hours_file;
    if (std::getenv("LEDGER_HIRES")) {
        units_file.open(prefix + "_units.csv");
        units_file << "trace,seat,label,day,unit,hire_hour,wage,turns,work,moves\n";
        units_out = &units_file;
        hours_file.open(prefix + "_hours.csv");
        hours_file << "trace,seat,label,day,hour,units,work,moves,idle,shed_io\n";
        hours_out = &hours_file;
    }
    if (std::getenv("LEDGER_SALES")) {
        sales.open(prefix + "_sales.csv");
        sales << "trace,seat,day,hour,product,units,revenue,inventory\n";
        sales_out = &sales;
    }
    std::ifstream list(argv[1]);
    int n = 0;
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2], group;
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        fields >> group;
        try {
            process(trace, labels, group, games, days, crops, animals);
            ++n;
        } catch (const std::exception& e) {
            std::cerr << trace << ": " << e.what() << '\n';
        }
    }
    std::cerr << "processed " << n << " traces\n";
    return 0;
}
