#include "compiler.hpp"
#include "opp_prior.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <numeric>
#include <cstdio>
#include <cstdlib>

namespace dc10 {

const char* status_name(CompileStatus s) {
    switch (s) {
        case CompileStatus::Ok: return "ok";
        case CompileStatus::InvalidIntent: return "invalid_intent";
        case CompileStatus::NoSchedule: return "no_schedule";
        case CompileStatus::Unfunded: return "unfunded";
    }
    return "?";
}

namespace {

// Members sorted by distance to the shed: work goes to the nearest members.
template <class Group>
std::vector<int> members_by_distance(const Group& g) {
    std::vector<int> cells(g.cells.begin(), g.cells.begin() + g.size);
    std::stable_sort(cells.begin(), cells.end(), [](int a, int b) { return shed_dist(a) < shed_dist(b); });
    return cells;
}

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

// Extra units one fertilizer applied today adds (active today and the next 2 days), assuming the
// crop is watered daily: one-shot crops double each yield-adding water until the cap (only today's
// if harvested today); ongoing crops double each production in the window.
int fertilizer_units(const CropGroup& g, int option, int day) {
    const auto& c = CROPS[g.crop];
    int extra = 0;
    if (c.ongoing) {
        for (int k = g.fert_days; k <= 2 && day + k < LAST_DAY; ++k) {
            const int since = g.age + k + 1 - c.first_yield_day;
            extra += since >= 0 && since % c.interval == 0 && since / c.interval < c.max_yield;
        }
        return extra;
    }
    const int first = (c.max_yield_day + 1) / 2, days = has_harvest(option) ? 1 : 3;
    int plain = g.yield, doubled = g.yield;
    for (int k = has_water(option) ? 0 : 1; k < days && day + k <= LAST_DAY; ++k) {
        if (g.age + k < first || g.age + k > c.max_yield_day) continue;
        plain = std::min<int>(c.max_yield, plain + (g.fert_days > k ? 2 : 1));
        doubled = std::min<int>(c.max_yield, doubled + 2);
    }
    return doubled - plain;
}

bool fertilizer_pays(const agent::AgentObservation& dawn, const CropGroup& g, int option, int day) {
    return fertilizer_units(g, option, day) * dawn.market.prices[g.crop] >= dawn.market.prices[FERTILIZER];
}

// Load leveling (CompileOptions::level_hires): work that loses nothing by waiting until tomorrow.
bool crop_can_wait(const CropGroup& g, int day) {
    const auto& c = CROPS[g.crop];
    if (day >= LAST_DAY - 1 || g.decaying) return false;
    if (!c.ongoing) return g.age + 1 <= c.max_yield_day;  // no yield loss tomorrow
    const int last = c.first_yield_day + c.interval * (c.max_yield - 1);  // last production age
    const int since = g.age + 1 - c.first_yield_day;
    const bool tonight = since >= 0 && since % c.interval == 0 && since / c.interval < c.max_yield;
    return g.age + 1 <= last && g.yield + (tonight ? 2 : 0) <= c.max_yield;
}

bool animal_can_wait(const AnimalGroup& g, int day) {
    const auto& a = ANIMALS[g.species];
    return day < LAST_DAY - 1 && g.held + 1 + g.bonus <= a.max_held;  // room for tonight's production and bank
}

struct Production { int units[N_PRODUCTS]{}; };

// Builds the solver input for one fallback level. Returns expected harvest output.
Production build_input(const agent::AgentObservation& dawn, const Schema& s, const DayIntent& in, int level,
                       const CompileOptions& options, int return_percent, dp::DayInput& out,
                       int fertilize_cap = 1 << 20) {
    out = dp::DayInput{};
    Production produced;
    const int day = s.day;
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        Tile t = tile_at(dawn, cell);
        t.planted_day = int16_t(t.planted_day - day);
        t.fertilized_until_day = int16_t(t.fertilized_until_day - day);
        t.max_lifespan_step = t.max_lifespan_step < 0 ? INT_MAX : t.max_lifespan_step - day * HOURS;
        out.grid[cell] = t;
    }
    for (int i = 0; i < N_ITEMS; ++i) out.shed[i] = dawn.own.shed[i];
    for (int c = 0; c < N_CROPS; ++c) out.seeds[c] = dawn.own.seeds[c];
    out.hours = last_hour(day) + 1;
    const bool survival = level >= SurvivalOnly;
    // Recovery: a cash crisis keeps the day's income (the network's harvests and collections) and
    // feeds from cash plus that income. Survival without income starved the farm (3 collapses in
    // 145 top-team replays; with recovery all 3 are wins). DC10_NO_RECOVERY: the old survival.
    const bool recovery = survival && options.recovery;
    const bool no_new = level >= NoNewEntities;
    int fertilize_events = 0, feed_events = 0;
    // Sites beyond open tiles, today's clears and harvests: one-shot crops that turn
    // into weeds today are dug (same end state as digging the weed) as needed.
    int weed_digs = 0;
    if (!no_new) {
        DayIntent held = in;
        held.buy_land = in.buy_land && level < NoLand;
        int sites = free_sites(s, held);
        for (int i = 0; i < s.n_crops; ++i)
            if (!s.crops[i].fresh && turns_weed_today(s.crops[i]))
                for (int o = 0; o < OPTIONS; ++o) sites -= (o == CLEAR || has_harvest(o)) ? 0 : in.options[i][o];
        weed_digs = std::max(0, new_entities(in) - sites);
    }

    for (int i = 0; i < s.n_crops; ++i) {
        const CropGroup& g = s.crops[i];
        if (g.fresh) continue;
        const auto cells = members_by_distance(g);
        if (!CROPS[g.crop].ongoing) {
            // Options with more actions go to nearer members.
            std::vector<int> order;
            for (int o = 0; o < OPTIONS; ++o)
                for (int n = 0; n < in.options[i][o]; ++n) order.push_back(o);
            std::stable_sort(order.begin(), order.end(), [](int a, int b) {
                auto work = [](int o) { return o == CLEAR ? 1 : std::popcount(unsigned(o)); };
                return work(a) > work(b);
            });
            for (size_t m = 0; m < cells.size(); ++m) {
                int o = order[m];
                if (survival) o = (recovery && has_harvest(o) ? 1 : 0) | ((has_water(o) && g.dry >= 1) ? 4 : 0);
                if (options.defer && has_harvest(o) && crop_can_wait(g, day)) {
                    o &= ~1;
                    const auto& c = CROPS[g.crop];
                    if (g.dry >= 1 || (g.age >= (c.max_yield_day + 1) / 2 && g.yield < c.max_yield)) o |= 4;  // alive, growing
                }
                if (has_fertilize(o) && (fertilize_events >= fertilize_cap ||
                                         (options.fert_value && !fertilizer_pays(dawn, g, o, day)))) o &= ~2;
                uint8_t e = 0;
                if (o == CLEAR) e = dp::Clear;
                else {
                    if (has_water(o)) e |= dp::Water;
                    if (has_fertilize(o)) e |= dp::Fertilize, ++fertilize_events;
                    if (has_harvest(o)) {
                        e |= dp::Harvest;
                        if (g.age <= CROPS[g.crop].max_yield_day) produced.units[g.crop] += g.yield + water_gain(g, o);
                    }
                    if (!has_harvest(o) && turns_weed_today(g) && weed_digs > 0) e = dp::Clear, --weed_digs;
                }
                out.events[cells[m]] = e;
            }
            continue;
        }
        // Ongoing: retained nearest, then cleared, then abandoned. Harvest prefers visited members.
        const int r = in.retain[i], c = in.clear[i];
        const bool terminal = day == LAST_DAY;
        for (size_t m = 0; m < cells.size(); ++m) {
            uint8_t e = 0;
            const bool retained = !terminal && int(m) < r;
            const bool cleared = !terminal && int(m) >= r && int(m) < r + c;
            const bool harvested = int(m) < in.harvest[i] && (!survival || recovery) && !(options.defer && crop_can_wait(g, day));
            const bool fertilized = int(m) < in.fertilize[i] && !survival && fertilize_events < fertilize_cap &&
                                    (!options.fert_value || fertilizer_pays(dawn, g, 7, day));
            if (cleared && !survival) e |= dp::Clear;
            if (fertilized) e |= dp::Fertilize, ++fertilize_events;
            if (harvested) {
                e |= dp::Harvest;
                if (!g.decaying) produced.units[g.crop] += g.yield;
            }
            if (retained) {
                const int held = harvested ? 0 : g.yield;
                const bool fertilizer_active = g.fert_days >= 1 || fertilized;
                const bool doubled = production_tonight(g, day) && fertilizer_active &&
                                     held + 2 <= CROPS[g.crop].max_yield;
                if (g.dry >= 1 || (doubled && options.water_for_production)) e |= dp::Water;
            }
            out.events[cells[m]] = e;
        }
    }
    // Survival with little cash: feed only as many animals as wheat on hand plus
    // affordable wheat allows; an idle day would lose every crop and animal.
    int feed_budget = 1 << 30;
    if (survival) {
        const double price = std::max(1, market_price(WHEAT, dawn.market.inventory[WHEAT] - 1));
        double income = 0;  // today's harvests at 80% of the current price (recovery)
        if (recovery)
            for (int p = 0; p < N_PRODUCTS; ++p) income += 0.8 * produced.units[p] * dawn.market.prices[p];
        feed_budget = int(dawn.own.shed[WHEAT]) + int((std::max(0.0, dawn.self().money) + income) / (price * 1.1));
    }
    for (int i = 0; i < s.n_animals; ++i) {
        const AnimalGroup& g = s.animals[i];
        if (g.fresh) continue;
        const auto cells = members_by_distance(g);
        const bool full = options.full_care && !survival && !care_fixed(g, day);
        for (size_t m = 0; m < cells.size(); ++m) {
            uint8_t e = 0;
            if ((full || int(m) < in.feed[i]) && feed_events < feed_budget) e |= dp::Feed, ++feed_events;
            if ((full || int(m) < in.care[i]) && !survival) e |= dp::Care;
            if (int(m) < in.collect[i] && (!survival || recovery) && !(options.defer && animal_can_wait(g, day))) {
                e |= dp::Harvest;
                produced.units[ANIMALS[g.species].product] += g.held;
            }
            if ((e || (options.collect_all && !survival)) && options.collect_fertilizer && level < NoCollection &&
                tile_at(dawn, cells[m]).fertilizer_available)
                e |= dp::CollectFertilizer;
            out.events[cells[m]] = e;
        }
    }
    int buy_animals[N_ANIMALS]{};
    if (!no_new) {
        for (int crop = 0; crop < N_CROPS; ++crop) {
            const int gi = s.new_crop_group[crop];
            CropGroup planted{};
            planted.crop = uint8_t(crop), planted.yield = 1;
            const bool pays = !options.fert_value || fertilizer_pays(dawn, planted, 6, day);
            for (int o = 0; o < OPTIONS; ++o)
                for (int n = 0; n < in.options[gi][o]; ++n) {
                    uint8_t e = dp::Water;
                    if (has_fertilize(o) && fertilize_events < fertilize_cap && pays) e |= dp::Fertilize, ++fertilize_events;
                    out.establish[out.establish_count++] = {uint8_t(crop), e};
                }
            if (CROPS[crop].ongoing)
                for (int n = 0; n < in.new_crop[crop]; ++n) out.establish[out.establish_count++] = {uint8_t(crop), dp::Water};
            out.buy_seeds[0][crop] = std::max(0, in.new_crop[crop] - int(dawn.own.seeds[crop]));
        }
        for (int a = 0; a < N_ANIMALS; ++a) {
            const int gi = s.new_animal_group[a];
            for (int n = 0; n < in.new_animal[a]; ++n) {
                uint8_t e = 0;
                if (n < in.feed[gi]) e |= dp::Feed, ++feed_events;
                if (n < in.care[gi]) e |= dp::Care;
                out.establish[out.establish_count++] = {uint8_t(GOOSE + a), e};
            }
            buy_animals[a] = in.new_animal[a] + in.reserve[a] - s.unplaced_dawn[a];
        }
    }
    for (int a = 0; a < N_ANIMALS; ++a) out.buy_animals[0][a] = std::max(0, buy_animals[a]);
    out.buy_wheat[0] = std::max(0, feed_events - int(dawn.own.shed[WHEAT]));
    out.buy_fertilizer[0] = std::max(0, fertilize_events - int(dawn.own.shed[FERTILIZER]));
    out.land_hour = (in.buy_land && level < NoLand) ? 0 : -1;
    // Day 29 returns by the last market hour: the most valuable units first.
    // Ordinary days return only night overflow (see compile_day).
    int total = 0, selected[N_PRODUCTS]{};
    for (int p = 0; p < N_PRODUCTS; ++p) total += p == FERTILIZER ? 0 : produced.units[p];
    for (int n = (total * return_percent + 99) / 100; n > 0; --n) {
        int best = -1;
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != FERTILIZER && selected[p] < produced.units[p] &&
                (best < 0 || market_price(p, dawn.market.inventory[p] + selected[p]) >
                                 market_price(best, dawn.market.inventory[best] + selected[best])))
                best = p;
        ++selected[best];
    }
    for (int p = 0; p < N_PRODUCTS; ++p)
        for (int h = out.hours - 1; h < HOURS; ++h) out.returns[h][p] = selected[p];
    return produced;
}

double sale_value(int item, int inventory, int units) {
    double v = 0;
    for (int k = 0; k < units; ++k) v += market_price(item, inventory + k);
    return v;
}

// Funding proposal: purchases move to the hour where projected cash covers them.
// Financing comes from sellable dawn stock, then fertilizer collected from animals
// and required harvest output, returned by `return_hour` (most valuable first).
// Hires are capped at what dawn cash covers. False if the bill cannot be covered.
bool fund_input(const agent::AgentObservation& dawn, const Production& produced, int return_hour,
                bool hires_first, dp::DayInput& in, int& max_hires) {
    auto bill_of = [&](int h) {
        double b = 0;
        for (int c = 0; c < N_CROPS; ++c) b += double(CROPS[c].seed) * in.buy_seeds[h][c];
        for (int a = 0; a < N_ANIMALS; ++a) b += double(ANIMALS[a].cost) * in.buy_animals[h][a];
        b += sale_value(WHEAT, dawn.market.inventory[WHEAT] - in.buy_wheat[h], in.buy_wheat[h]);
        b += sale_value(FERTILIZER, dawn.market.inventory[FERTILIZER] - in.buy_fertilizer[h], in.buy_fertilizer[h]);
        if (in.land_hour == h) b += LAND_PRICES[std::min(2, dawn.self().n_quadrants - 1)];
        return b;
    };
    // Cash at hour 0: money plus sellable dawn stock (inputs needed today are kept).
    int needed[N_PRODUCTS]{};
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        needed[WHEAT] += bool(in.events[cell] & dp::Feed);
        needed[FERTILIZER] += bool(in.events[cell] & dp::Fertilize);
    }
    for (int n = 0; n < in.establish_count; ++n) {
        needed[WHEAT] += bool(in.establish[n].events & dp::Feed);
        needed[FERTILIZER] += bool(in.establish[n].events & dp::Fertilize);
    }
    double cash = dawn.self().money;
    for (int p = 0; p < N_PRODUCTS; ++p)
        cash += sale_value(p, dawn.market.inventory[p], std::max(0, int(dawn.own.shed[p]) - needed[p]));
    // Financing receipts by return_hour.
    double later = 0;
    int returned[N_PRODUCTS]{};
    for (int cell = 0; cell < BOARD * BOARD; ++cell)
        if (tile_at(dawn, cell).has_animal && tile_at(dawn, cell).fertilizer_available) {
            in.events[cell] |= dp::CollectFertilizer;
            ++returned[FERTILIZER];
        }
    for (int p = 0; p < N_PRODUCTS; ++p) returned[p] += p == FERTILIZER ? 0 : produced.units[p];
    // Purchases at hour 0 in survival-first order; the rest wait for the returns.
    const double total = bill_of(0);
    if (total <= 0) return true;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        const int keep = p == FERTILIZER ? std::max(0, needed[FERTILIZER] - int(dawn.own.shed[FERTILIZER])) : 0;
        later += sale_value(p, dawn.market.inventory[p], std::max(0, returned[p] - keep));
    }
    if (cash + later < total) return false;
    dp::DayInput original = in;
    double available = cash;
    // Hires first: wages are cheap early and the workers collect the financing output.
    // Hire while dawn cash pays the wage and the day's receipts still cover every purchase.
    max_hires = 0;
    if (hires_first) {
        double spare = cash + later - total;
        while (max_hires < 13 && available >= fib(max_hires) && spare >= fib(max_hires)) {
            spare -= fib(max_hires);
            available -= fib(max_hires++);
        }
    }
    auto defer = [&](int& now, int& late, double price) {
        const int affordable = price > 0 ? std::min(now, int(available / price)) : now;
        available -= affordable * price;
        late += now - affordable;
        now = affordable;
    };
    defer(in.buy_wheat[0], in.buy_wheat[return_hour], market_price(WHEAT, dawn.market.inventory[WHEAT] - 1));
    defer(in.buy_fertilizer[0], in.buy_fertilizer[return_hour], market_price(FERTILIZER, dawn.market.inventory[FERTILIZER] - 1));
    for (int c = 0; c < N_CROPS; ++c) defer(in.buy_seeds[0][c], in.buy_seeds[return_hour][c], CROPS[c].seed);
    if (in.land_hour == 0) {
        const double land = LAND_PRICES[std::min(2, dawn.self().n_quadrants - 1)];
        if (available >= land) available -= land;
        else in.land_hour = return_hour;
    }
    for (int a = 0; a < N_ANIMALS; ++a) defer(in.buy_animals[0][a], in.buy_animals[return_hour][a], ANIMALS[a].cost);
    // Otherwise hires are paid at hour 0 from what remains of dawn cash.
    if (!hires_first)
        while (max_hires < 13 && available >= fib(max_hires)) available -= fib(max_hires++);
    // Return the financing sources, most valuable first, until the deferred bill is covered.
    double deficit = bill_of(return_hour) - available;
    int inventory[N_PRODUCTS];
    for (int p = 0; p < N_PRODUCTS; ++p) inventory[p] = dawn.market.inventory[p];
    int target[N_PRODUCTS]{};
    target[FERTILIZER] = returned[FERTILIZER];  // collected fertilizer is sale capital
    deficit -= sale_value(FERTILIZER, inventory[FERTILIZER], returned[FERTILIZER]);
    inventory[FERTILIZER] += returned[FERTILIZER];
    while (deficit > 0) {
        int best = -1;
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (target[p] < returned[p] && (best < 0 || market_price(p, inventory[p]) > market_price(best, inventory[best])))
                best = p;
        if (best < 0) {
            in = original;
            return false;
        }
        deficit -= market_price(best, inventory[best]++);
        ++target[best];
    }
    for (int p = 0; p < N_PRODUCTS; ++p)
        for (int h = return_hour; h < HOURS; ++h) in.returns[h][p] = std::max(in.returns[h][p], target[p]);
    return true;
}

bool is_purchase(uint8_t op) { return op == M_BUY_SEED || op == M_BUY_ANIMAL || op == M_BUY_PRODUCT; }

// Moves every purchase to the hour before its first use in the solver's schedule.
// An engine pass supplies missing items as the schedule needs them; each supplied
// amount becomes a purchase one hour earlier. Reserve animals are bought last.
bool retime_purchases(const agent::AgentObservation& dawn, const Schema& s, const DayIntent& in, int level,
                      DayPlan& plan) {
    Config config;
    Sim sim = sim_from_observation(dawn, config);
    const int me = dawn.player;
    sim.st.farms[me].money = 1e12;  // item timing only; funding is checked separately
    Action pass;
    pass.finalize();
    int seeds[HOURS][N_CROPS]{}, items[HOURS][N_ITEMS]{};
    int bought_animals[N_ANIMALS]{};
    for (int h = 0; h < plan.hours; ++h) {
        Action a = plan.actions[h];
        int kept = 0;
        for (int k = 0; k < a.n_orders; ++k)
            if (!is_purchase(a.orders[k].op)) a.orders[kept++] = a.orders[k];
        a.n_orders = kept;
        Farm& f = sim.st.farms[me];
        const int units = std::min(a.n_units, f.n_units);
        int plant[N_CROPS]{}, pickup[N_ITEMS]{};
        for (int u = 0; u < units; ++u) {
            if (a.units[u].op == OP_PLANT && a.units[u].arg < N_CROPS) ++plant[a.units[u].arg];
            if (a.units[u].op == OP_PICKUP && a.units[u].arg < N_ITEMS) pickup[a.units[u].arg] += a.units[u].n;
        }
        for (int c = 0; c < N_CROPS; ++c)
            if (plant[c] > f.seeds[c]) {
                if (h == 0) return false;
                seeds[h - 1][c] += plant[c] - f.seeds[c];
                f.seeds[c] = Count(plant[c]);
            }
        for (int i : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)})
            if (pickup[i] > f.shed[i]) {
                if (h == 0) return false;
                const int d = pickup[i] - f.shed[i];
                items[h - 1][i] += d;
                f.shed[i] = Count(pickup[i]);
                f.shed_total += d;
                if (is_animal(uint8_t(i))) bought_animals[i - GOOSE] += d;
            }
        me == 0 ? sim.step(a, pass) : sim.step(pass, a);
    }
    if (level < NoNewEntities)
        for (int a = 0; a < N_ANIMALS; ++a)
            items[plan.hours - 1][GOOSE + a] +=
                std::max(0, in.new_animal[a] + in.reserve[a] - s.unplaced_dawn[a] - bought_animals[a]);
    for (int h = 0; h < plan.hours; ++h) {
        Action& a = plan.actions[h];
        int kept = 0;
        for (int k = 0; k < a.n_orders; ++k)
            if (!is_purchase(a.orders[k].op)) a.orders[kept++] = a.orders[k];
        a.n_orders = kept;
        for (int c = 0; c < N_CROPS; ++c)
            if (seeds[h][c]) a.orders[a.n_orders++] = {M_BUY_SEED, uint8_t(c), seeds[h][c]};
        for (int i = 0; i < N_ITEMS; ++i)
            if (items[h][i])
                a.orders[a.n_orders++] = {uint8_t(is_animal(uint8_t(i)) ? M_BUY_ANIMAL : M_BUY_PRODUCT), uint8_t(i), items[h][i]};
        if (a.n_orders > 10) return false;
    }
    return true;
}

// Cost of this hour's planned purchases, hires and land at current prices.
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

// Orders for one hour: sales first, then the plan's purchases and hires.
void merge_orders(const Action& planned, const int sell[N_PRODUCTS], int max_orders, Action& action,
                  const agent::AgentObservation& obs, int sell_order) {
    action.n_orders = 0;
    int slots = max_orders - planned.n_orders;
    int order[N_PRODUCTS];
    std::iota(order, order + N_PRODUCTS, 0);
    double key[N_PRODUCTS];
    for (int p = 0; p < N_PRODUCTS; ++p)
        key[p] = sell_order == 1 ? obs.market.prices[p]
                                 : double(market_price(p, obs.market.inventory[p]) -
                                          market_price(p, obs.market.inventory[p] + sell[p])) * sell[p];
    if (sell_order) std::stable_sort(order, order + N_PRODUCTS, [&](int a, int b) { return key[a] > key[b]; });
    for (int k = 0; k < N_PRODUCTS && slots > 0; ++k)
        if (const int p = order[k]; sell[p] > 0) {
            action.orders[action.n_orders++] = {M_SELL, uint8_t(p), sell[p]};
            --slots;
        }
    for (int k = 0; k < planned.n_orders; ++k) action.orders[action.n_orders++] = planned.orders[k];
}

// Runs the plan in an engine copy with a passive opponent; false if a purchase or hire fails.
// Opponent output visible on its farm now: held animal products and harvestable crops.

// CompileOptions::forecast_fit: coefficients (trailing, visible, stock, sell-through x (visible + stock),
// day / 30, 1) per product, fitted by tools/forecast_audit rows (FORECAST_ROWS) on 600 on-policy games
// (reports/value_data, Sep 25).
constexpr double FORECAST_FIT[N_PRODUCTS][6] = {
    {0, 0, 0, 0, 0, 0},                                       // wheat: trailing only
    {-0.0244, -0.0077, 0.9766, 0.0409, 0.0228, -0.0084},      // carrot
    {0.0194, 0.0041, 0.9987, -0.0005, -0.0385, 0.0131},       // tomato
    {0.7552, 0.2545, 0.7149, -0.2062, 1.3505, -0.3041},       // strawberry
    {-0.0006, 0.7359, 0.6489, 0.4380, 0.0008, -0.0040},       // melon
    {-0.0533, -0.0282, 0.9668, 0.0857, 0.1577, -0.0302},      // egg
    {0.2635, 0.2251, 0.5319, -0.1765, 6.3437, -0.7765},       // milk
    {-0.0125, 0.3442, 0.5496, -0.0131, 4.9130, -0.6515},      // wool
    {0, 0, 0, 0, 0, 0},                                       // fertilizer: trailing only
};

void fit_forecast(const agent::AgentObservation& o, const History& history, double rival[HOURS][N_PRODUCTS]) {
    if (o.day >= LAST_DAY) return;
    int visible[N_PRODUCTS];
    visible_supply(o, visible);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (p == WHEAT || p == FERTILIZER) continue;
        double total = 0;
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
        const double stock = history.opponent_stock()[p], supply = visible[p] + stock;
        const auto& c = FORECAST_FIT[p];
        const double target = std::max(0.0, c[0] * total + c[1] * visible[p] + c[2] * stock +
                                                 c[3] * history.sell_through(o.day, p, 3, 0.6) * supply + c[4] * o.day / 30.0 + c[5]);
        for (int h = 0; h < HOURS; ++h) {
            const double share = total > 0 ? std::max(0.0, rival[h][p]) / total : (h >= 6 && h <= 12 ? 1.0 / 7 : 0.0);
            rival[h][p] = rival[h][p] < 0 ? rival[h][p] : target * share;
        }
    }
}

double fit_value(int p, double trailing, double visible, double stock, double through, int day) {
    const auto& c = FORECAST_FIT[p];
    return std::max(0.0, c[0] * trailing + c[1] * visible + c[2] * stock + c[3] * through * (visible + stock) +
                             c[4] * day / 30.0 + c[5]);
}

void blend_forecast(const agent::AgentObservation& o, double rival[HOURS][N_PRODUCTS], double w,
                    const std::array<int, N_PRODUCTS>* stock, bool prior);

// CompileOptions::forecast_select applied to a trailing forecast (w: blend weight of visible supply).
void select_forecast(const agent::AgentObservation& o, const History& history, double rival[HOURS][N_PRODUCTS], double w,
                     bool prior = false) {
    if (o.day >= LAST_DAY) return;
    double blended[HOURS][N_PRODUCTS], fitted[HOURS][N_PRODUCTS];
    std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &blended[0][0]);
    std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &fitted[0][0]);
    blend_forecast(o, blended, w, nullptr, prior);
    fit_forecast(o, history, fitted);
    double error[2][N_PRODUCTS]{};  // blend, fit
    for (int k = std::max(1, o.day - 3); k < o.day; ++k) {
        double past[HOURS][N_PRODUCTS];
        history.expected(k, 3, past);
        for (int p = 0; p < N_PRODUCTS; ++p) {
            double trailing = 0;
            for (int h = 0; h < HOURS; ++h) trailing += std::max(0.0, past[h][p]);
            const double visible = history.dawn_visible(k, p), stock = history.dawn_stock(k, p);
            const double blend = p == MELON ? visible : (1 - w) * trailing + w * visible;
            const double fit = fit_value(p, trailing, visible, stock, history.sell_through(k, p, 3, 0.6), k);
            error[0][p] += std::abs(blend - history.sold_on(k, p));
            error[1][p] += std::abs(fit - history.sold_on(k, p));
        }
    }
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (p == WHEAT || p == FERTILIZER) continue;
        const auto& chosen = error[1][p] < error[0][p] ? fitted : blended;
        for (int h = 0; h < HOURS; ++h) rival[h][p] = chosen[h][p];
    }
}

// CompileOptions::forecast_timing: keep each product's forecast total, spread it over the hours like
// all of this opponent's past sales of the product.
void reshape_forecast(const agent::AgentObservation& o, const History& history, double rival[HOURS][N_PRODUCTS]) {
    if (o.day >= LAST_DAY) return;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (p == WHEAT || p == FERTILIZER) continue;
        double profile[HOURS], total = 0, mass = 0;
        history.hour_profile(o.day, p, profile);
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]), mass += profile[h];
        if (mass <= 0 || total <= 0) continue;
        for (int h = 0; h < HOURS; ++h)
            if (rival[h][p] >= 0) rival[h][p] = total * profile[h];
    }
}

// CompileOptions::forecast_adapt applied to a trailing forecast.
void adapt_forecast(const agent::AgentObservation& o, const History& history, double rival[HOURS][N_PRODUCTS]) {
    if (o.day >= LAST_DAY) return;
    int visible[N_PRODUCTS];
    visible_supply(o, visible);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (p == WHEAT || p == FERTILIZER) continue;
        double total = 0;
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
        const double target = history.sell_through(o.day, p, 3, 0.6) * (visible[p] + history.opponent_stock()[p]);
        for (int h = 0; h < HOURS; ++h) {
            const double share = total > 0 ? std::max(0.0, rival[h][p]) / total : (h >= 6 && h <= 12 ? 1.0 / 7 : 0.0);
            rival[h][p] = rival[h][p] < 0 ? rival[h][p] : target * share;
        }
    }
}

// CompileOptions::forecast_blend applied to a trailing forecast (w: weight of visible supply).
void blend_forecast(const agent::AgentObservation& o, double rival[HOURS][N_PRODUCTS], double w,
                    const std::array<int, N_PRODUCTS>* stock = nullptr, bool prior = false) {
    if (o.day >= LAST_DAY) return;
    if (prior) {  // melons, milk, wool: blend total, hourly shape from the strong-player prior
        int visible[N_PRODUCTS];
        visible_supply(o, visible);
        const int bucket = o.day <= 9 ? 0 : o.day <= 14 ? 1 : o.day <= 22 ? 2 : 3;
        for (const int p : {int(MELON), int(MILK), int(WOOL)}) {
            double total = 0;
            for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
            const bool seen = total > 0;
            const double daily = p == MELON ? visible[p] : (1 - w) * total + w * visible[p];
            for (int h = 0; h < HOURS; ++h) {
                const double shape = seen ? 0.5 * std::max(0.0, rival[h][p]) / total + 0.5 * OPP_PRIOR[p][bucket][1][h]
                                          : OPP_PRIOR[p][bucket][0][h];
                rival[h][p] = daily * shape;
            }
        }
    }
    int visible[N_PRODUCTS];
    visible_supply(o, visible);
    if (stock)
        for (int p = 0; p < N_PRODUCTS; ++p) visible[p] += std::max(0, (*stock)[p]);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (p == WHEAT || p == FERTILIZER || (prior && (p == MELON || p == MILK || p == WOOL))) continue;
        double total = 0;
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
        const double target = p == MELON ? visible[p] : (1 - w) * total + w * visible[p];
        for (int h = 0; h < HOURS; ++h) {
            const double share = total > 0 ? std::max(0.0, rival[h][p]) / total : (h >= 6 && h <= 12 ? 1.0 / 7 : 0.0);
            rival[h][p] = rival[h][p] < 0 ? rival[h][p] : target * share;
        }
    }
}


// stress: the opponent also sells all visible output at hour 2 (funding safety).
bool funded(const agent::AgentObservation& dawn, const History& history, const DayPlan& plan,
            const CompileOptions& options, bool stress, std::string& why, double* margin = nullptr,
            int* room_item = nullptr, double* shortfall = nullptr) {
    Config config;
    Sim sim = sim_from_observation(dawn, config);
    DayExecutor executor;
    executor.start(dawn, history, plan, options);
    double rival[HOURS][N_PRODUCTS];
    history.expected(dawn.day, 3, rival);
    if (stress) {
        int visible[N_PRODUCTS];
        visible_supply(dawn, visible);
        for (int p = 0; p < N_PRODUCTS; ++p) rival[std::min(2, plan.hours - 1)][p] += visible[p];
    }
    const int me = dawn.player;
    // Shortfall (budget revision): the largest excess of an hour's planned purchases over the cash
    // then, or the gap to the next-dawn reserve.
    double worst = 50;
    for (int h = 0; h < plan.hours; ++h) {
        const auto obs = agent::runtime::make_observation(sim, me);
        worst = std::max(worst, planned_cost(obs, plan.actions[h]) - obs.self().money);
        Action mine;
        executor.act(obs, mine);
        // Expected opponent: sells its forecast quantities in the first order slots.
        Action pass;
        Farm& opponent = sim.st.farms[1 - me];
        pass.n_units = opponent.n_units;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            const int n = int(std::lround(std::max(0.0, rival[h][p])));
            if (!n || pass.n_orders >= 10) continue;
            opponent.shed[p] = Count(n);
            pass.orders[pass.n_orders++] = {M_SELL, uint8_t(p), n};
        }
        pass.finalize();
        const auto accepted = me == 0 ? sim.sanitize_joint_actions(mine, pass)[0] : sim.sanitize_joint_actions(pass, mine)[1];
        const auto diag = (me == 0 ? sim.diagnose_joint_actions(mine, pass) : sim.diagnose_joint_actions(pass, mine)).players[me];
        if (diag.successful_unit_actions != diag.requested_unit_actions) {
            if (std::getenv("DC10_DEBUG")) {
                std::fprintf(stderr, "unit fail h%d money %.0f seeds:", h, obs.self().money);
                for (int c = 0; c < N_CROPS; ++c) std::fprintf(stderr, " %d", obs.own.seeds[c]);
                std::fprintf(stderr, " shed:");
                for (int i = 0; i < N_ITEMS; ++i) std::fprintf(stderr, " %d", obs.own.shed[i]);
                std::fprintf(stderr, " | units (op:arg:n@cell):");
                for (int u = 0; u < mine.n_units; ++u)
                    std::fprintf(stderr, " %d:%d:%d@%d", mine.units[u].op, mine.units[u].arg, mine.units[u].n,
                                 obs.self().pos_y[u] * BOARD + obs.self().pos_x[u]);
                std::fprintf(stderr, " | orders:");
                for (int k = 0; k < mine.n_orders; ++k) std::fprintf(stderr, " %d:%d:%d", mine.orders[k].op, mine.orders[k].item, mine.orders[k].n);
                std::fprintf(stderr, " | planned orders prev hour:");
                if (h) for (int k = 0; k < plan.actions[h - 1].n_orders; ++k)
                    std::fprintf(stderr, " %d:%d:%d", plan.actions[h - 1].orders[k].op, plan.actions[h - 1].orders[k].item, plan.actions[h - 1].orders[k].n);
                std::fprintf(stderr, "\n");
            }
            why = "unit action fails at hour " + std::to_string(h);
            if (shortfall) *shortfall = worst;
            return false;
        }
        for (int k = 0; k < mine.n_orders; ++k) {
            const auto& o = mine.orders[k];
            if (o.op == M_SELL) continue;
            const int want = (o.op == M_HIRE || o.op == M_BUY_LAND) ? 1 : o.n;
            if (accepted.orders[k].n < want || accepted.orders[k].op != o.op) {
                if (std::getenv("DC10_DEBUG")) {
                    std::fprintf(stderr, "unfunded h%d cash %.0f cost %.0f shed:", h, obs.self().money, planned_cost(obs, plan.actions[h]));
                    for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d", obs.own.shed[p]);
                    std::fprintf(stderr, " | orders:");
                    for (int j = 0; j < mine.n_orders; ++j) std::fprintf(stderr, " %d:%d:%d", mine.orders[j].op, mine.orders[j].item, mine.orders[j].n);
                    std::fprintf(stderr, "\n");
                }
                // A purchase that fails with enough cash lacks shed room or an order slot for
                // the sale that makes room: not a cash failure.
                if (room_item && o.op != M_HIRE && o.op != M_BUY_LAND && obs.self().money >= planned_cost(obs, plan.actions[h]))
                    *room_item = o.item;
                why = "order " + std::to_string(int(o.op)) + " item " + std::to_string(int(o.item)) + " short at hour " +
                      std::to_string(h);
                if (shortfall) *shortfall = worst;
                return false;
            }
        }
        me == 0 ? sim.step(mine, pass) : sim.step(pass, mine);
    }
    if (margin) *margin = sim.st.farms[me].money - sim.st.farms[1 - me].money;
    // Keep tomorrow's first feed affordable from cash plus sellable goods: a farm that
    // ends a day with neither cannot hire or feed, no plan routes, and every crop and
    // animal dies (full-game failure vs teammate_shoprouter).
    // From day 1 (day 0: new animals are fed today and make fertilizer tonight; the day-0 reserve
    // cost 3 melons or the third sheep). The cash crunches this allows later are handled by the
    // recovery level (build_input). options.reserve_from_day: another first day.
    if (plan.fallback < SurvivalOnly && !std::getenv("DC10_NO_RESERVE") && dawn.day >= options.reserve_from_day) {
        int animals = 0;
        for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(dawn, cell).has_animal;
        for (int a = 0; a < N_ANIMALS; ++a) animals += plan.new_animals[a];
        const Farm& end = sim.st.farms[me];
        // DC10_RESERVE_UNFED: only animals unfed today must be fed tomorrow (an animal
        // escapes after two unfed days); the state here is after tonight's update.
        if (std::getenv("DC10_RESERVE_UNFED")) {
            animals = 0;
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) animals += end.tiles[y][x].has_animal && end.tiles[y][x].consecutive_dry >= 1;
        }
        const double price = market_price(WHEAT, sim.st.market.inventory[WHEAT] - 1);
        const double reserve = std::max(0, animals - int(end.shed[WHEAT])) * price * 1.2 + 30;
        double assets = end.money;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            if (p == WHEAT) continue;
            int units = end.shed[p];
            for (int u = 0; u < end.n_units; ++u) units += end.inv[u][p];
            assets += 0.8 * units * sim.st.market.prices[p];
        }
        // DC10_RESERVE_FARM: also the output waiting on the farm after tonight (fertilizer made
        // by every animal, product held by animals), which tomorrow's plan collects and sells.
        if (std::getenv("DC10_RESERVE_FARM"))
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    const Tile& t = end.tiles[y][x];
                    if (!t.has_animal) continue;
                    if (t.fertilizer_available) assets += 0.8 * sim.st.market.prices[FERTILIZER];
                    if (t.yield_units > 0) assets += 0.8 * t.yield_units * sim.st.market.prices[ANIMALS[t.what - GOOSE].product];
                }
        if (assets < reserve && end.money < dawn.self().money) {
            why = "end assets " + std::to_string(int(assets)) + " below next-dawn reserve " + std::to_string(int(reserve));
            if (shortfall) *shortfall = reserve - assets;
            return false;
        }
    }
    return true;
}
}

namespace {
int receipt_deadline(const agent::AgentObservation& dawn, const double rival[HOURS][N_PRODUCTS], int p, int shed,
                     int units, int hours, bool tie_now);
double return_value(const agent::AgentObservation& dawn, const double rival[HOURS][N_PRODUCTS], int p, int carried,
                    int n, int deadline, int hours, double hold_discount, int* sold_today = nullptr, bool tie_now = false);
}

namespace {
DayPlan compile_once(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent,
                     const CompileOptions& options, dp::Solver& solver, int max_level = SurvivalOnly);

// Removes one unit of the most expensive new entity (sheep, cow, goose, then crops by
// seed price), keeping the intent valid. False when nothing new remains.
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

// Budget revision: removes the new crops and animals whose purchase cost covers `need` dollars at
// the least log-probability loss under the decoder's preferences (in.drop_loss): an exact
// covering knapsack over units in $10 steps. False if the intent has no preferences or nothing
// can cover the need.
bool revise_for_budget(const Schema& s, DayIntent& in, double need) {
    if (!in.has_drop_loss || need <= 0) return false;
    const int target = int(std::ceil(need / 10));
    constexpr double INF = 1e18;
    std::vector<double> best(target + 1, INF);
    best[0] = 0;
    std::vector<std::vector<int>> taken(8, std::vector<int>(target + 1, 0));
    for (int i = 0; i < 8; ++i) {
        const int units = std::min(32, i < N_CROPS ? int(in.new_crop[i]) : int(in.new_animal[i - N_CROPS]));
        const int cost = (i < N_CROPS ? CROPS[i].seed : ANIMALS[i - N_CROPS].cost) / 10;
        std::vector<double> next(target + 1, INF);
        for (int b = 0; b <= target; ++b) {
            double loss = 0;
            for (int k = 0; k <= units; ++k) {
                if (k) loss += in.drop_loss[i][k - 1];
                const double v = best[std::max(0, b - k * cost)] + loss;
                if (v < next[b]) next[b] = v, taken[i][b] = k;
            }
        }
        best = next;
    }
    if (best[target] >= INF) return false;
    int removed[8], b = target;
    for (int i = 7; i >= 0; --i) {
        removed[i] = taken[i][b];
        b = std::max(0, b - removed[i] * ((i < N_CROPS ? CROPS[i].seed : ANIMALS[i - N_CROPS].cost) / 10));
    }
    for (int i = 0; i < 8; ++i)
        for (int k = 0; k < removed[i]; ++k) i < N_CROPS ? drop_crop(s, in, i) : drop_animal(s, in, i - N_CROPS);
    if (std::getenv("DC10_DEBUG")) {
        std::fprintf(stderr, "budget revision: need $%.0f, loss %.2f, removed crops", need, best[target]);
        for (int i = 0; i < 8; ++i) std::fprintf(stderr, i == N_CROPS ? " animals %d" : " %d", removed[i]);
        std::fprintf(stderr, "\n");
    }
    return true;
}

bool trim_new_entity(const Schema& s, DayIntent& in, const CompileOptions& options) {
    auto animal_of = [&](int a) { return drop_animal(s, in, a); };
    auto crop_of = [&](int c) { return drop_crop(s, in, c); };
    // DC10_TRIM_MODEL: the network decides what to give up (the units it is least likely to want),
    // each step verified by a full compile: replays +4.1k vs the rules below, but 6/22 head to head
    // vs the herd agent (keeps fewer day-0 crops with the day-0 reserve).
    if (options.trim_model)
        while (in.trim_next < in.trim_count) {
            const int i = in.trim_order[in.trim_next++];
            if (i < N_CROPS ? in.new_crop[i] > 0 : in.new_animal[i - N_CROPS] > 0)
                return i < N_CROPS ? crop_of(i) : animal_of(i - N_CROPS);
        }
    auto animal = [&] {
        for (int a : {2, 1, 0})
            if (in.new_animal[a] > 0) return animal_of(a);
        return false;
    };
    auto crop = [&] {
        int order[N_CROPS] = {0, 1, 2, 3, 4};
        std::sort(order, order + N_CROPS, [](int a, int b) { return CROPS[a].seed > CROPS[b].seed; });
        for (int c : order)
            if (in.new_crop[c] > 0) return crop_of(c);
        return false;
    };
    // Day 0 keeps the requested herd as long as crops can go (dropping the day-0 sheep cost the
    // first wool income and cascaded into day-6 trims: +6.6k, wins 85% -> 100% on unseen seeds);
    // DC10_TRIM_D0_ANIMALS: the old order. DC10_TRIM_CROPS_FIRST: crops first on every day.
    if (s.day == 0 && !std::getenv("DC10_TRIM_D0_ANIMALS")) return crop() || animal();
    if (std::getenv("DC10_TRIM_CROPS_FIRST")) return crop() || animal();
    return animal() || crop();
}
}

// Fallback before dropping every new entity: trim new entities one unit at a time
// (most expensive first) while the result would otherwise lose all of them.
// DC10_PROFILE: time per compile phase (ms), printed to stderr after each compile_day.
struct Profile {
    double solve = 0, ladder = 0, funded = 0, retime = 0, build = 0, trims = 0;
    int solves = 0, fundeds = 0;
};
thread_local Profile profile;
struct Timed {
    double& total;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~Timed() { total += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(); }
};

DayPlan compile_day(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent,
                    const CompileOptions& options, dp::Solver& solver) {
    profile = Profile{};
    // DC10_LAND_FIRST: on a land day, trim new entities (keeping the land) before the ladder
    // drops the land.
    if (intent.buy_land && std::getenv("DC10_LAND_FIRST") && !std::getenv("DC10_NO_TRIM")) {
        DayPlan kept = compile_once(dawn, history, intent, options, solver, KeepAll);
        const Schema schema = describe(dawn);
        DayIntent trimmed = intent;
        double spent = kept.compile_ms;
        for (int trims = 0; trims <= 12; ++trims) {
            if (trims) {
                if (!trim_new_entity(schema, trimmed, options)) break;
                kept = compile_once(dawn, history, trimmed, options, solver, KeepAll);
                spent += kept.compile_ms;
            }
            if (kept.status == CompileStatus::Ok && kept.fallback == KeepAll) {
                if (trims) kept.reason = "land kept, trimmed " + std::to_string(trims) + " new entities;" + kept.reason;
                kept.fallback = trims ? int(NoLand) : KeepAll;  // requirements were dropped
                kept.compile_ms = spent;
                return kept;
            }
        }
    }
    DayPlan plan = compile_once(dawn, history, intent, options, solver);
    if (options.slack_care && !options.full_care && !options.time_pressure && plan.status == CompileStatus::Ok &&
        plan.fallback == KeepAll && dawn.day < LAST_DAY) {
        CompileOptions cared = options;
        cared.full_care = true;
        cared.solve.max_hires = plan.hires;
        DayPlan fuller = compile_once(dawn, history, intent, cared, solver);
        if (fuller.status == CompileStatus::Ok && fuller.fallback == KeepAll && fuller.hires <= plan.hires) {
            fuller.reason = "slack care;" + fuller.reason;
            fuller.compile_ms += plan.compile_ms;
            plan = fuller;
        } else {
            plan.compile_ms += fuller.compile_ms;
        }
    }
    if (options.level_hires && plan.status == CompileStatus::Ok && plan.hires >= options.level_hires) {
        CompileOptions deferred = options;
        deferred.defer = true;
        deferred.solve.max_hires = plan.hires - 1;
        DayPlan lighter = compile_once(dawn, history, intent, deferred, solver);
        if (lighter.status == CompileStatus::Ok && lighter.fallback <= plan.fallback && lighter.hires < plan.hires) {
            lighter.reason = "deferred work (hires " + std::to_string(plan.hires) + " -> " + std::to_string(lighter.hires) +
                             ");" + lighter.reason;
            lighter.compile_ms += plan.compile_ms;
            return lighter;
        }
        plan.compile_ms += lighter.compile_ms;
    }
    const bool lost_new = plan.status != CompileStatus::Ok || plan.fallback >= NoNewEntities;
    if (!lost_new || std::getenv("DC10_NO_TRIM")) return plan;
    const Schema schema = describe(dawn);
    double spent = plan.compile_ms;
    // DC10_BUDGET_REVISION: the network's preferences choose what to give up. The funding check's
    // shortfall is covered at the least log-probability loss (revise_for_budget). The check stops
    // at the first failure, so a retry can reveal more: each round re-solves from the original
    // intent for the cumulative shortfall (a joint choice, not per-round increments).
    if (std::getenv("DC10_BUDGET_REVISION")) {
        double need = plan.shortfall;
        DayIntent revised = intent;
        for (int round = 1; round <= 5 && need > 0 && revise_for_budget(schema, revised = intent, need); ++round) {
            DayPlan retry = [&] {
                Timed t{profile.trims};
                return compile_once(dawn, history, revised, options, solver, NoLand);
            }();
            spent += retry.compile_ms;
            if (retry.status == CompileStatus::Ok && retry.fallback < NoNewEntities) {
                retry.reason = plan.reason + "budget revision " + std::to_string(round) + " (short $" +
                               std::to_string(int(need)) + ");" + retry.reason;
                retry.fallback = std::max(retry.fallback, int(NoLand));  // requirements were dropped
                retry.compile_ms = spent;
                return retry;
            }
            if (retry.shortfall <= 0) break;
            need += retry.shortfall;
        }
    }
    DayIntent trimmed = intent;
    // Deterministic budget: a fixed number of trims (no wall-clock limit, so results do
    // not depend on machine load).
    for (int trims = 1; trims <= 12 && trim_new_entity(schema, trimmed, options); ++trims) {
        if (std::getenv("DC10_DEBUG")) {
            std::fprintf(stderr, "trim %d: crops", trims);
            for (int c = 0; c < N_CROPS; ++c) std::fprintf(stderr, " %d", trimmed.new_crop[c]);
            std::fprintf(stderr, " animals");
            for (int a = 0; a < N_ANIMALS; ++a) std::fprintf(stderr, " %d", trimmed.new_animal[a]);
            std::fprintf(stderr, "\n");
        }
        // Only the levels that keep new entities: deeper levels were tried on the
        // untrimmed intent and do not depend on new entities.
        DayPlan retry = [&] {
            Timed t{profile.trims};
            return compile_once(dawn, history, trimmed, options, solver, NoLand);
        }();
        spent += retry.compile_ms;
        if (retry.status == CompileStatus::Ok && retry.fallback < NoNewEntities) {
            retry.reason = plan.reason + "trimmed " + std::to_string(trims) + " new entities;" + retry.reason;
            retry.fallback = std::max(retry.fallback, int(NoLand));  // requirements were dropped
            retry.compile_ms = spent;
            return retry;
        }
    }
    plan.compile_ms = spent;
    return plan;
}

void print_profile(int day, double total) {
    if (!std::getenv("DC10_PROFILE")) return;
    std::fprintf(stderr, "profile day %d total %.0f solve %.0f (%d) ladder %.0f funded %.0f (%d) retime %.0f build %.0f trims %.0f\n", day,
                 total, profile.solve, profile.solves, profile.ladder, profile.funded, profile.fundeds, profile.retime,
                 profile.build, profile.trims);
    auto& s = dp::solver_profile();
    std::fprintf(stderr, "solver day %d setup %.0f prepare %.0f execute %.0f (%ld) retry %.0f (%ld)\n", day, s.setup, s.prepare,
                 s.execute, s.executions, s.retry_execute, s.retry_executions);
    std::fprintf(stderr, "routes day %d setup %.0f init %.0f improve %.0f reorder %.0f refine %.0f reassign %.0f\n", day,
                 s.route_setup, s.route_init, s.route_improve, s.route_reorder, s.reorder_refine, s.reorder_reassign);
    s = dp::SolverProfile{};
}

namespace {
DayPlan compile_once(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent,
                     const CompileOptions& options, dp::Solver& solver, int max_level) {
    const auto t0 = std::chrono::steady_clock::now();
    DayPlan plan;
    double first_shortfall = 0;  // first failed stress check (budget revision)
    Schema schema = describe(dawn);
    size_new_groups(schema, intent);
    const std::string invalid = validate(schema, intent);
    if (!invalid.empty()) {
        plan.status = CompileStatus::InvalidIntent;
        plan.reason = invalid;
    } else {
        const bool terminal = dawn.day == LAST_DAY;
        const int quotas_terminal[] = {100, 75, 50, 25, 0};
        const int quotas_ordinary[] = {0};
        const int* quotas = terminal ? quotas_terminal : quotas_ordinary;
        const int quota_count = terminal ? 5 : 1;
        // Funding variants: 0 plain; 1-3 purchases first, 4-6 hires first, with
        // financing returns by hour 3, 6 or 10.
        const int variants = 7, return_hours[] = {-1, 3, 6, 10, 3, 6, 10};
        bool unfunded = false;
        int fertilize_cap = 1 << 20;  // lowered once when bought fertilizer has no shed room
        DayPlan expected_only;
        expected_only.hours = 0;
        for (int attempt = 0; attempt < FallbackLevels * quota_count * variants; ++attempt) {
            const int funding = attempt % variants;
            const int level = attempt / (quota_count * variants), quota = quotas[(attempt / variants) % quota_count];
            if (level > max_level) break;
            if (funding == 0 && expected_only.hours && expected_only.fallback < level) {
                plan = expected_only;  // keep requirements; funded under the expected forecast only
                plan.status = CompileStatus::Ok;
                break;
            }
            if (level == NoLand && !intent.buy_land) continue;
            if (level >= NoCollection && quota) continue;
            if (funding == 0) unfunded = false;
            else if (!unfunded) continue;  // funding variants only after a funding failure
            const Production produced = [&] {
                Timed t{profile.build};
                return build_input(dawn, schema, intent, level, options, quota, plan.input, fertilize_cap);
            }();
            if (terminal && quota) {
                double rival[HOURS][N_PRODUCTS];
                history.expected(dawn.day, 3, rival);
                int visible[N_PRODUCTS];
                visible_supply(dawn, visible);
                for (int p = 0; p < N_PRODUCTS; ++p)
                    for (int h = 1; h <= 12; ++h) rival[h][p] += visible[p] / 12.0;
                for (int p = 0; p < N_PRODUCTS; ++p) {
                    const int units = plan.input.returns[HOURS - 1][p];
                    if (!units) continue;
                    const int due = receipt_deadline(dawn, rival, p, dawn.own.shed[p], units, plan.input.hours, (options.sale_tie_now || (options.melon_tie_now && p == MELON)));
                    for (int h = due; h < HOURS; ++h) plan.input.returns[h][p] = units;
                }
            }
            auto solve_options = options.solve;
            solve_options.feasibility_first = !std::getenv("DC10_HIRE_SCAN");  // DC10_HIRE_SCAN: ascending hire scan
            if (options.time_pressure >= 2) solve_options.max_executions = 150;  // emergency
            if ((options.route_search == 1 || options.route_search == 3) && !options.time_pressure && options.time_left > 40) {
                const bool wide = options.route_search == 1;  // 3: medium search (8 route, 4 workforce variants)
                solve_options.variants = wide ? 16 : 8, solve_options.minimize_variants = wide ? 8 : 4;
                solve_options.opportunistic_hire_reduction = true;
            }
            if (!std::getenv("DC10_NO_HIRE_CAP")) {  // never plan more hires than dawn cash can pay (wages are due when hiring)
                int affordable = 0;
                double cash = dawn.self().money;
                if (options.hire_cap_stock)
                    for (int p = 0; p < N_PRODUCTS; ++p)
                        if (p != WHEAT && p != FERTILIZER)
                            cash += 0.9 * sale_value(p, dawn.market.inventory[p], dawn.own.shed[p]);
                for (double money = cash; affordable < solve_options.max_hires && money >= fib(affordable);)
                    money -= fib(affordable++);
                solve_options.max_hires = affordable;
            }
            if (funding) {
                if (!fund_input(dawn, produced, std::min(return_hours[funding], plan.input.hours - 2), funding > 3, plan.input,
                                solve_options.max_hires)) {
                    plan.reason += "L" + std::to_string(level) + "F" + std::to_string(funding) + ":bill exceeds cash;";
                    continue;
                }
            }
            auto timed_solve = [&](const dp::DayInput& input) {
                Timed t{profile.solve};
                ++profile.solves;
                return solver.solve(input, solve_options);
            };
            auto result = timed_solve(plan.input);
            if (options.route_search == 2 && !options.time_pressure && options.time_left > 40 &&
                result.status == dp::SolveStatus::Success && result.hires >= 12) {
                solve_options.variants = 16, solve_options.minimize_variants = 8;
                solve_options.opportunistic_hire_reduction = true;
                const auto wider = timed_solve(plan.input);
                if (wider.status == dp::SolveStatus::Success && wider.hires < result.hires) result = wider;
            }
            if (result.status != dp::SolveStatus::Success && terminal && quota) {
                // Early receipt deadlines may be unroutable: keep the quota, drop the deadlines.
                for (int p = 0; p < N_PRODUCTS; ++p)
                    for (int h = 0; h < plan.input.hours - 1; ++h) plan.input.returns[h][p] = 0;
                result = timed_solve(plan.input);
            }
            plan.fallback = level;
            plan.return_percent = quota;
            for (int a = 0; a < N_ANIMALS; ++a) plan.new_animals[a] = level < NoNewEntities ? intent.new_animal[a] : 0;
            if (result.status != dp::SolveStatus::Success) {
                if (std::getenv("DC10_DEBUG")) {
                    std::fprintf(stderr, "no route L%d F%d ms %.0f land_hour %d max_hires %d returns@23:", level, funding,
                                 result.microseconds / 1000, plan.input.land_hour, solve_options.max_hires);
                    for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d", plan.input.returns[HOURS - 1][p]);
                    std::fprintf(stderr, " first_return_hour:");
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        int first = -1;
                        for (int h = 0; h < HOURS && first < 0; ++h)
                            if (plan.input.returns[h][p]) first = h;
                        std::fprintf(stderr, " %d", first);
                    }
                    int empty = 0, cleared = 0, weeds = 0;
                    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
                        const Tile& t = plan.input.grid[cell];
                        const uint8_t e = plan.input.events[cell];
                        empty += t.kind == T_EMPTY;
                        weeds += t.kind == T_WEED;
                        cleared += (e & dp::Clear) || (t.kind == T_PLANT && !CROPS[t.what].ongoing && (e & dp::Harvest));
                    }
                    std::fprintf(stderr, " | establish %d empty %d cleared %d weeds %d quadrants %d\n", plan.input.establish_count,
                                 empty, cleared, weeds, dawn.self().n_quadrants);
                }
                plan.status = CompileStatus::NoSchedule;
                plan.reason += "L" + std::to_string(level) + (result.status == dp::SolveStatus::InvalidInput ? ":solver rejected input;" : ":no route found;");
                continue;
            }
            // Returns of carried output, driven by market opportunity and shed capacity.
            // Units still carried at the end of the route enter the shed tonight and
            // overflow is destroyed (the v7 suffix search measured this the same way).
            // For each product, the sale DP values n carried units reaching the shed by
            // hour 20 (sold today in stages or held) against carrying them overnight;
            // capacity then adds the most valuable units until tonight's deposit fits,
            // due by the last hour (as top players do: carry up to the room, deposit
            // and sell the rest at hours 22-23).
            if (!terminal) {
                int carried[N_PRODUCTS]{}, total = 0;
                for (int u = 0; u < result.state.worker_count; ++u)
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        carried[p] += result.state.workers[u].inventory[p];
                        total += result.state.workers[u].inventory[p];
                    }
                const int deadline = std::min(options.market_deadline, plan.input.hours - 2);
                double rival[HOURS][N_PRODUCTS];
                history.expected(dawn.day, 3, rival);
                if (options.forecast_select)
                    select_forecast(dawn, history, rival, options.blend_visible / 100.0, options.forecast_prior);
                else if (options.forecast_fit) fit_forecast(dawn, history, rival);
                else if (options.forecast_adapt) adapt_forecast(dawn, history, rival);
                else if (options.forecast_blend)
                    blend_forecast(dawn, rival, options.blend_visible / 100.0,
                                   options.forecast_stock ? &history.opponent_stock() : nullptr, options.forecast_prior);
                if (options.forecast_timing) reshape_forecast(dawn, history, rival);
                // DC10_RACE: melons have no shop demand, so the first seller of a harvest wave takes the
                // price: the opponent's visible ripe melons count as supply early (it sells at hour
                // ~9) and our market part is due by hour 10 (DC10_RACE_DEADLINE). Replays +4.8 win
                // points, Local-LB +1.1k, not better head to head: optional.
                const bool race = options.race;
                if (race) {
                    int visible[N_PRODUCTS];
                    visible_supply(dawn, visible);
                    rival[std::min(2, plan.input.hours - 1)][MELON] += visible[MELON];
                }
                // Market part: the carried units the sale DP would sell today if all were
                // returned by hour 20 (a returned unit can also be held, so returning
                // everything never looks worse; count only units it actually sells).
                int want[N_PRODUCTS]{}, wanted = 0;
                double gain = 0;  // sale DP value of the market part over holding it
                double none_value[N_PRODUCTS]{};
                for (int p = 0; p < N_PRODUCTS; ++p) {
                    const int c = carried[p];
                    if (!c) continue;
                    const double route_cost = 2;  // per returned unit: extra trips can cost other work
                    int sold = 0;
                    const double none = return_value(dawn, rival, p, c, 0, deadline, plan.input.hours, options.hold_discount,
                                                     nullptr, (options.sale_tie_now || (options.melon_tie_now && p == MELON)));
                    none_value[p] = none;
                    const double all = return_value(dawn, rival, p, c, c, deadline, plan.input.hours, options.hold_discount,
                                                    &sold, (options.sale_tie_now || (options.melon_tie_now && p == MELON)));
                    const int n = std::clamp(sold - int(dawn.own.shed[p]), 0, c);
                    if (n && all - none > route_cost * n) want[p] = n, gain += all - none;
                    wanted += want[p];
                }
                // Tonight's room: the shed can be sold empty except tomorrow's first feed
                // (1 wheat per animal) not already carried in tonight's deposit.
                int animals = 0;
                for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(dawn, cell).has_animal;
                for (int a = 0; a < N_ANIMALS; ++a) animals += intent.new_animal[a];
                const int room = std::max(0, 95 - std::max(0, animals - carried[WHEAT]));
                // Capacity part: carried units beyond tonight's room, spread over products in
                // proportion to what is carried, so every worker returns part of its load
                // (one product harvested late and far away may be unreturnable).
                int need[N_PRODUCTS]{};
                if (total > room)
                    for (int p = 0; p < N_PRODUCTS; ++p) need[p] = (carried[p] * (total - room) + total - 1) / total;
                // Attempts: the market part by hour 20 with the capacity part by the last
                // hour (workers deposit before that hour's market, so it is still sold);
                // then the capacity part alone, then 75/50/25% of it. Late harvests cannot
                // be returned early, so capacity returns are due at the last hour.
                const int last = plan.input.hours - 1;
                // 0: market and capacity; -1 (race_dp): the same with per-product delivery hours.
                std::vector<int> ladder = {0, 100, 75, 50, 25};
                const int* market = want;
                int dp_deadline[N_PRODUCTS];
                std::fill_n(dp_deadline, N_PRODUCTS, deadline);
                double dp_gain = 0;
                if (options.race_dp && options.time_left > 30) {
                    ladder.insert(ladder.begin(), -1);
                    if (options.race_steps) ladder.insert(ladder.begin() + 1, {-2, -3, -4, -5, -6});
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        if (!market[p]) continue;
                        const int candidates[] = {options.race_early ? 6 : 10, options.race_early ? 8 : 10, 10, 12, 14, 16, 18, 20};
                        double values[8], best = -1e18;
                        for (int k = 0; k < 8; ++k) {
                            values[k] = return_value(dawn, rival, p, carried[p], market[p], std::min(candidates[k], deadline),
                                                     plan.input.hours, options.hold_discount, nullptr, (options.sale_tie_now || (options.melon_tie_now && p == MELON)));
                            best = std::max(best, values[k]);
                        }
                        for (int k = 7; k >= 0; --k)
                            if (values[k] >= best - std::max(20.0, 0.01 * std::abs(best))) {
                                dp_deadline[p] = std::min(candidates[k], deadline);
                                dp_gain += values[k] - none_value[p];
                                break;
                            }
                    }
                }
                for (const int percent : ladder) {
                    const bool market_attempt = percent <= 0;
                    int units = 0;
                    for (int p = 0; p < N_PRODUCTS; ++p) units += !market_attempt ? need[p] * percent / 100 : std::max(market[p], need[p]);
                    if (!units || (market_attempt && !wanted)) continue;
                    if (market_attempt && options.time_pressure >= 2) continue;  // emergency: capacity returns only
                    const double spent = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                    dp::DayInput repaired = plan.input;
                    const int race_hour = options.race_deadline;
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        const int received = result.receipts[plan.input.hours - 1][p];
                        const int early = !market_attempt ? 0 : market[p];
                        const int late = !market_attempt ? need[p] * percent / 100 : std::max(market[p], need[p]);
                        const int from = percent < 0 && early ? std::min(deadline, dp_deadline[p] + options.race_steps * (-percent - 1))
                                         : race && p == MELON && early ? std::min(deadline, race_hour) : deadline;
                        for (int h = from; h < HOURS; ++h) {
                            const int target = h >= last ? late : early;
                            if (target) repaired.returns[h][p] = std::max(repaired.returns[h][p], received + target);
                        }
                    }
                    const int due = !market_attempt ? last : deadline;
                    // Returns only add work: start the hire search at the base plan's count
                    // (DC10_NO_WARM_START: the full search from the work estimate).
                    const auto base_solve_options = solve_options;
                    if (!std::getenv("DC10_NO_WARM_START")) solve_options.min_hires = result.hires;
                    if (options.time_pressure >= 2) solve_options.effort = dp::SearchEffort::Fast;
                    if (market_attempt) {  // the market attempt often fails after a long search
                        if (options.time_pressure >= 1) solve_options.effort = dp::SearchEffort::Fast;
                        if (const char* e = std::getenv("DC10_LADDER_EFFORT"))
                            solve_options.effort = std::string(e) == "fast" ? dp::SearchEffort::Fast : dp::SearchEffort::Compact;
                        if (const char* cap = std::getenv("DC10_LADDER_MAX_EXEC")) solve_options.max_executions = std::atoi(cap);
                    }
                    const auto returned = [&] {
                        Timed t{profile.ladder};
                        return timed_solve(repaired);
                    }();
                    solve_options = base_solve_options;
                    if (std::getenv("DC10_DEBUG")) {
                        std::fprintf(stderr, "return L%d F%d carried %d room %d wanted %d percent %d due %d status %d spent %.0f ms %.0f attempts %d executions %d hires %d (base %d, %d attempts) | carried/market/need:",
                                     level, funding, total, room, wanted, percent, due, int(returned.status), spent, returned.microseconds / 1000,
                                     returned.attempts, returned.executions, returned.hires, result.hires, result.attempts);
                        for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d/%d/%d", carried[p], market[p], need[p]);
                        std::fprintf(stderr, "\n");
                    }
                    double wages = 0;  // extra wages of the hires the returns add
                    for (int k = result.hires; k < returned.hires; ++k) wages += fib(k);
                    if (returned.status == dp::SolveStatus::Success && market_attempt && options.return_wages &&
                        (percent < 0 ? dp_gain : gain) < wages)
                        continue;  // the market part does not pay its wages: capacity returns only
                    if (returned.status == dp::SolveStatus::Success) {
                        plan.input = repaired;
                        result = returned;
                        plan.return_percent = !market_attempt ? -percent : -1;  // -1: market and capacity; -percent: capacity part
                        break;
                    }
                }
            }
            plan.hours = plan.input.hours;
            plan.hires = result.hires;
            std::copy_n(result.schedule, HOURS, plan.actions);
            std::copy_n(&result.receipts[0][0], HOURS * N_PRODUCTS, &plan.receipts[0][0]);
            plan.night_carried = 0;
            std::fill_n(plan.night_items, N_PRODUCTS, 0);
            for (int u = 0; u < result.state.worker_count; ++u)
                for (int i = 0; i < N_ITEMS; ++i) {
                    plan.night_carried += result.state.workers[u].inventory[i];
                    if (i < N_PRODUCTS) plan.night_items[i] += result.state.workers[u].inventory[i];
                }
            DayPlan retimed = plan;
            if ([&] {
                    Timed t{profile.retime};
                    return retime_purchases(dawn, schema, intent, level, retimed);
                }())
                plan = retimed;
            else plan.reason += "L" + std::to_string(level) + ":retime failed;";
            std::string why;
            int room_item = -1;
            double shortfall = 0;
            auto timed_funded = [&](bool stress, int* room) {
                Timed t{profile.funded};
                ++profile.fundeds;
                return funded(dawn, history, plan, options, stress, why, nullptr, room, stress ? &shortfall : nullptr);
            };
            if (!timed_funded(true, &room_item)) {
                if (!first_shortfall) first_shortfall = shortfall;
                plan.reason += "L" + std::to_string(level) + "F" + std::to_string(funding) + ":stress " + why + ";";
                if (!expected_only.hours && timed_funded(false, nullptr)) expected_only = plan;
                if (!timed_funded(false, nullptr)) plan.reason += "expected " + why + ";";
                plan.status = CompileStatus::Unfunded;
                unfunded = room_item < 0;  // funding variants only help cash failures
                // Bought fertilizer without shed room (or without an order slot for the sale
                // that makes room): retry once with fertilize events limited to the dawn
                // stock plus the dawn room left after hour-0 wheat and animal purchases.
                if (room_item == FERTILIZER && fertilize_cap == 1 << 20) {
                    int room = 100 - int(dawn.own.shed_total) - plan.input.buy_wheat[0];
                    for (int a = 0; a < N_ANIMALS; ++a) room -= plan.input.buy_animals[0][a];
                    fertilize_cap = int(dawn.own.shed[FERTILIZER]) + std::max(0, room);
                    plan.reason += "fertilize capped at " + std::to_string(fertilize_cap) + ";";
                    --attempt;
                    continue;
                }
                // Last variant of this level: accept a plan funded only under the expected forecast.
                if (funding == variants - 1 && expected_only.hours) {
                    plan = expected_only;
                    plan.status = CompileStatus::Ok;
                    plan.stress_funded = false;
                    break;
                }
                continue;
            }
            plan.stress_funded = true;
            plan.funding = funding;
            plan.status = CompileStatus::Ok;
            break;
        }
        if (plan.status != CompileStatus::Ok && expected_only.hours) {
            plan = expected_only;
            plan.status = CompileStatus::Ok;
        }
        // Last resort (design: a failed plan never becomes an idle day): survival work
        // on the event cells nearest the shed only, within what the affordable workers
        // can route. Unfunded risk is accepted: survival must happen.
        for (const int cap : {20, 12, 6}) {
            if (plan.status == CompileStatus::Ok || max_level < SurvivalOnly) break;
            DayPlan light;
            build_input(dawn, schema, intent, SurvivalOnly, options, 0, light.input);
            std::vector<int> cells;
            for (int cell = 0; cell < BOARD * BOARD; ++cell)
                if (light.input.events[cell]) cells.push_back(cell);
            std::stable_sort(cells.begin(), cells.end(), [](int a, int b) { return shed_dist(a) < shed_dist(b); });
            for (size_t k = cap; k < cells.size(); ++k) light.input.events[cells[k]] = 0;
            auto solve_options = options.solve;
            int affordable = 0;
            for (double money = dawn.self().money; affordable < solve_options.max_hires && money >= fib(affordable);)
                money -= fib(affordable++);
            solve_options.max_hires = affordable;
            solve_options.feasibility_first = !std::getenv("DC10_HIRE_SCAN");
            const auto result = solver.solve(light.input, solve_options);
            if (result.status != dp::SolveStatus::Success) continue;
            light.hours = light.input.hours;
            light.hires = result.hires;
            std::copy_n(result.schedule, HOURS, light.actions);
            std::copy_n(&result.receipts[0][0], HOURS * N_PRODUCTS, &light.receipts[0][0]);
            for (int u = 0; u < result.state.worker_count; ++u)
                for (int i = 0; i < N_ITEMS; ++i) {
                    light.night_carried += result.state.workers[u].inventory[i];
                    if (i < N_PRODUCTS) light.night_items[i] += result.state.workers[u].inventory[i];
                }
            DayPlan retimed = light;
            if (retime_purchases(dawn, schema, intent, SurvivalOnly, retimed)) light = retimed;
            light.status = CompileStatus::Ok;
            light.fallback = SurvivalOnly;
            light.reason = plan.reason + "survival capped at " + std::to_string(cap) + " cells;";
            plan = light;
        }
    }
    plan.shortfall = first_shortfall;
    plan.compile_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return plan;
}
}

void DayExecutor::start(const agent::AgentObservation& dawn, const History& history, const DayPlan& plan,
                        const CompileOptions& options) {
    plan_ = plan;
    options_ = options;
    day_ = dawn.day;
    history_ = &history;
    history.expected(dawn.day, 3, rival_);
    if (options.forecast_select) select_forecast(dawn, history, rival_, options.blend_visible / 100.0, options.forecast_prior);
    else if (options.forecast_fit) fit_forecast(dawn, history, rival_);
    else if (options.forecast_adapt) adapt_forecast(dawn, history, rival_);
    else if (options.forecast_blend)
        blend_forecast(dawn, rival_, options.blend_visible / 100.0, options.forecast_stock ? &history.opponent_stock() : nullptr,
                       options.forecast_prior);
    if (options.forecast_timing) reshape_forecast(dawn, history, rival_);
    std::fill_n(hold_supply_, N_PRODUCTS, 0);
    hold_discount_ = options.hold_discount;
    if (dawn.day == LAST_DAY - 1) {
        // Stock held tonight is liquidated tomorrow, after the opponent's visible output.
        visible_supply(dawn, hold_supply_);
        hold_discount_ = 0.95;
    }
    if (dawn.day == LAST_DAY) {
        // The opponent must liquidate too: its visible output reaches the market early.
        int visible[N_PRODUCTS];
        visible_supply(dawn, visible);
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int h = 1; h <= 12; ++h) rival_[h][p] += visible[p] / 12.0;
    }
}

void DayExecutor::act(const agent::AgentObservation& obs, Action& action) {
    action.clear();
    action.n_units = obs.self().n_units;
    for (int u = 0; u < action.n_units; ++u) action.units[u] = UnitAction{};
    const int h = obs.hour;
    if (obs.day != day_ || h >= plan_.hours) {
        action.finalize();
        return;
    }
    const Action& planned = plan_.actions[h];
    for (int u = 0; u < std::min(action.n_units, planned.n_units); ++u) action.units[u] = planned.units[u];
    // Inputs kept back: the peak stock that today's later pickups need beyond the
    // purchases usable by then (a purchase is usable from the next hour), then
    // tomorrow's first pickups beyond tonight's deposit (wheat: 1 per animal).
    // Deposits do not offset it: the seller may sell them.
    int reserve[N_PRODUCTS]{};
    int overnight[N_PRODUCTS]{};
    if (obs.day < LAST_DAY) {
        int animals = 0;
        for (int cell = 0; cell < BOARD * BOARD; ++cell) animals += tile_at(obs, cell).has_animal;
        overnight[WHEAT] = std::max(0, std::max(animals, plan_.keep_overnight[WHEAT]) - plan_.night_items[WHEAT]);
        overnight[FERTILIZER] = std::max(0, plan_.keep_overnight[FERTILIZER] - plan_.night_items[FERTILIZER]);
    }
    for (int p : {int(WHEAT), int(FERTILIZER)}) {
        int running = 0;
        for (int t = h; t < plan_.hours; ++t) {
            for (int u = 0; u < plan_.actions[t].n_units; ++u)  // pickups precede the market
                if (plan_.actions[t].units[u].op == OP_PICKUP && plan_.actions[t].units[u].arg == p)
                    running += plan_.actions[t].units[u].n;
            reserve[p] = std::max(reserve[p], running);
            for (int k = 0; k < plan_.actions[t].n_orders; ++k)
                if (plan_.actions[t].orders[k].op == M_BUY_PRODUCT && plan_.actions[t].orders[k].item == p)
                    running -= plan_.actions[t].orders[k].n;
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
    if (const char* debug = std::getenv("DC10_SALE_DEBUG"); debug && obs.hour == std::atoi(debug))
        std::fprintf(stderr, "night_room %d night_carried %d fixed_end %d reserve w%d f%d\n", night_room, plan_.night_carried, fixed_end, reserve[WHEAT], reserve[FERTILIZER]);
    int sell[N_PRODUCTS]{};
    // Expected opponent sales: its usual flow; on day 29 also the stock it holds now,
    // expected at this market (it must liquidate too, so we race it). Spreading the
    // stock over later hours, or using it on earlier days, did worse on dev.
    double rival[HOURS][N_PRODUCTS];
    std::copy_n(&rival_[0][0], HOURS * N_PRODUCTS, &rival[0][0]);
    if (options_.forecast_intraday && history_ && h > 0)
        for (int p = 0; p < N_PRODUCTS; ++p) {
            if (p == WHEAT || p == FERTILIZER) continue;
            double total = 0, later = 0;
            for (int t = 0; t < HOURS; ++t) total += std::max(0.0, rival_[t][p]);
            for (int t = h; t < HOURS; ++t) later += std::max(0.0, rival_[t][p]);
            const double left = std::max(0.0, total - history_->sold_on(obs.day, p));
            if (later > 0)
                for (int t = h; t < HOURS; ++t) rival[t][p] = rival_[t][p] < 0 ? rival_[t][p] : rival_[t][p] * left / later;
        }
    const int spread = std::getenv("DC10_STOCK_HOURS") ? std::atoi(std::getenv("DC10_STOCK_HOURS")) : 1;
    const char* stock_from = std::getenv("DC10_RIVAL_STOCK_FROM");
    if (history_ && !std::getenv("DC10_NO_RIVAL_STOCK") && obs.day >= (stock_from ? std::atoi(stock_from) : LAST_DAY))
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT && p != FERTILIZER)
                for (int k = 0; k < spread; ++k) rival[(h + k) % HOURS][p] += history_->opponent_stock()[p] / double(spread);
    if (options_.race && obs.day < LAST_DAY) {  // race the opponent's ripe and held melons
        int visible[N_PRODUCTS];
        visible_supply(obs, visible);
        rival[h][MELON] += visible[MELON] + (history_ ? history_->opponent_stock()[MELON] : 0);
    }
    choose_sales(obs, plan_.receipts, plan_.hours, hold_discount_, reserve, night_room, rival, hold_supply_, sell,
                 options_.sale_tie_now, options_.melon_tie_now);
    // Cover this hour's purchases: sell more of the highest-priced stock if needed.
    // The margin absorbs opponent orders that fill before ours at lower prices.
    const double base_cost = planned_cost(obs, planned);
    const double cost = base_cost > 0 ? base_cost * 1.05 + 20 + options_.cash_margin : 0;
    double cash = obs.self().money;
    int stock[N_PRODUCTS]{};
    for (int p = 0; p < N_PRODUCTS; ++p) {
        stock[p] = obs.own.shed[p];
        for (int k = 0; k < sell[p]; ++k) cash += market_price(p, obs.market.inventory[p] + k);
    }
    while (cash < cost) {
        int best = -1;
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (stock[p] - reserve[p] > sell[p] &&
                (best < 0 || market_price(p, obs.market.inventory[p] + sell[p]) >
                                 market_price(best, obs.market.inventory[best] + sell[best])))
                best = p;
        if (best < 0) break;
        cash += market_price(best, obs.market.inventory[best] + sell[best]);
        ++sell[best];
    }
    // Shed capacity: after this market, leave room for the next worker deposits
    // (or tonight's automatic deposit) and for this hour's purchases.
    int deposits_now = 0, pickups_now = 0, deposits_next = 0, bought = 0;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        deposits_now += plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0);
        if (h + 1 < plan_.hours) deposits_next += plan_.receipts[h + 1][p] - plan_.receipts[h][p];
    }

    for (int u = 0; u < std::min(obs.self().n_units, planned.n_units); ++u)
        if (planned.units[u].op == OP_PICKUP) pickups_now += std::min<int>(planned.units[u].n, obs.own.shed[planned.units[u].arg]);
    for (int k = 0; k < planned.n_orders; ++k)
        if (planned.orders[k].op == M_BUY_ANIMAL || planned.orders[k].op == M_BUY_PRODUCT) bought += planned.orders[k].n;
    int shed = obs.own.shed_total + deposits_now - pickups_now + bought;
    for (int p = 0; p < N_PRODUCTS; ++p) shed -= std::min(sell[p], stock[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0));
    const bool terminal = obs.day == LAST_DAY;
    (void)terminal;
    while (shed + deposits_next > 100) {
        // Sell the unit whose sale loses least against holding it until tomorrow.
        int best = -1;
        double best_loss = 1e18;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            const int available = stock[p] + plan_.receipts[h][p] - (h ? plan_.receipts[h - 1][p] : 0);
            if (available - reserve[p] <= sell[p]) continue;
            const int now = market_price(p, obs.market.inventory[p] + sell[p]);
            const double loss = options_.hold_discount * market_price(p, obs.market.inventory[p] - 30) - now;
            if (loss < best_loss) best_loss = loss, best = p;
        }
        if (best < 0) break;
        ++sell[best];
        --shed;
    }
    if (const char* debug = std::getenv("DC10_SALE_DEBUG"); debug && obs.hour == std::atoi(debug)) {
        std::fprintf(stderr, "sell:");
        for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d", sell[p]);
        std::fprintf(stderr, " planned_orders %d\n", planned.n_orders);
    }
    // Runtime cash guard: never let this hour's seed/animal purchases leave less than a
    // few dollars for tomorrow's hires. The dawn forecast cannot foresee every opponent
    // (vs teammate_shoprouter day 0 ended at exactly $0 and the farm died). Feed wheat
    // and hires are kept.
    Action guarded = planned;
    if (!std::getenv("DC10_NO_CASH_GUARD") && obs.day < LAST_DAY) {
        double cost_all = planned_cost(obs, planned);
        int kept = 0;
        for (int k = 0; k < planned.n_orders; ++k) {
            const auto& o = planned.orders[k];
            const bool optional = o.op == M_BUY_SEED || o.op == M_BUY_ANIMAL;
            if (optional && cash - cost_all < 8) {
                Action one;
                one.n_orders = 1;
                one.orders[0] = o;
                cost_all -= planned_cost(obs, one);
                continue;
            }
            guarded.orders[kept++] = o;
        }
        guarded.n_orders = kept;
    }
    merge_orders(guarded, sell, 10, action, obs, options_.sell_order);
    action.finalize();
}

// ---------------------------------------------------------------- sales
namespace {
// Market inventory removed by town and shops at each future step of today.
void demand_by_step(const agent::AgentObservation& obs, int item, int steps, int out[]) {
    for (int k = 0; k < steps; ++k) {
        const int step = obs.step + k;
        int d = 0;
        if (step % 4 == 0)
            for (int s = 0; s < obs.n_shops; ++s)
                if (SHOP_MASK[obs.shops[s]] & (1u << item)) d += SHOP_MULT[obs.shops[s]];
        if (step % HOURS == 0 && item != FERTILIZER) d += 1;
        out[k] = d;
    }
}

double revenue(int item, int inventory, int units) {
    double total = 0;
    for (int k = 0; k < units; ++k) total += market_price(item, inventory + k);
    return total;
}
}

namespace {
// Sale plan for one product over today's remaining markets. A per-unit charge on
// stock left after the last market lets the caller enforce night shed capacity.
struct ProductSale {
    int product = 0, total = 0, remaining = 0, inv0 = 0;
    int avail[HOURS]{};
    int cum_demand[2 * HOURS + 1]{};
    int rival_units[HOURS]{};  // expected opponent sales at each remaining hour
    double rival_weight = 0;   // margin weight of the opponent's revenue (1 before day 29, 0 on day 29)
    double hold_discount = 0;
    double hour_discount = 1;  // value of a later market relative to this one (forecast risk)
    int hold_supply = 0;       // units reaching the market before held units are sold tomorrow
    bool spot_hold = false;    // value held units at today's market instead of the recovered forecast
    bool terminal = false;
    bool tie_now = false;      // equal value: sell now
    double value = 0;          // optimal value from this hour with nothing sold yet
    std::vector<int> choice;

    int solve(double charge, int& first) {
        const int width = total + 1;
        std::vector<double> next(width), cur(width);
        choice.assign(remaining * width, 0);
        for (int s = 0; s <= total; ++s) {
            const int left = total - s;
            const int hold_inventory = spot_hold ? inv0 + s + hold_supply
                                                 : inv0 + s - cum_demand[remaining + HOURS / 2] + hold_supply;
            next[s] = terminal || !left ? 0 : hold_discount * revenue(product, hold_inventory, left);
            next[s] -= charge * left;
        }
        for (int k = remaining - 1; k >= 0; --k) {
            for (int s = 0; s <= total; ++s) {
                const int inventory = inv0 + s - cum_demand[k];
                // Opponent revenue after our sale this hour, at its expected quantity.
                auto rival_cost = [&](int q) {
                    return rival_weight && rival_units[k] ? rival_weight * revenue(product, inventory + q, rival_units[k]) : 0.0;
                };
                double best = hour_discount * next[s] - rival_cost(0), gained = 0;
                int best_q = 0;
                for (int q = 1; q <= avail[k] - s; ++q) {
                    gained += market_price(product, inventory + q - 1);
                    const double value = gained + hour_discount * next[s + q] - rival_cost(q);
                    if (value > best || (tie_now && value >= best - 1e-6)) best = value, best_q = q;
                }
                cur[s] = best;
                choice[k * width + s] = best_q;
            }
            std::swap(cur, next);
        }
        value = next[0];
        int sold = 0;
        for (int k = 0; k < remaining; ++k) {
            const int q = choice[k * width + sold];
            if (!k) first = q;
            sold += q;
        }
        return total - sold;
    }
};
}

namespace {
// Value of today's sales plus held stock when n of `carried` units reach the shed by
// `deadline` and the rest arrive tonight, from dawn with the expected opponent flow.
double return_value(const agent::AgentObservation& dawn, const double rival[HOURS][N_PRODUCTS], int p, int carried,
                    int n, int deadline, int hours, double hold_discount, int* sold_today, bool tie_now) {
    ProductSale sale;
    sale.tie_now = tie_now;
    sale.product = p;
    sale.remaining = hours;
    sale.inv0 = dawn.market.inventory[p];
    sale.hold_discount = hold_discount;
    sale.rival_weight = 1;
    int demand[2 * HOURS]{};
    demand_by_step(dawn, p, hours + HOURS / 2, demand);
    double net = 0;
    for (int k = 0; k < hours + HOURS / 2; ++k) {
        const double opponent = rival[k % HOURS][p];
        if (k < hours) sale.rival_units[k] = int(std::lround(std::max(0.0, opponent)));
        net += demand[k] - (p == WHEAT || p == FERTILIZER ? opponent : std::max(0.0, opponent));
        sale.cum_demand[k + 1] = int(std::lround(net));
    }
    for (int k = 0; k < hours; ++k) sale.avail[k] = dawn.own.shed[p] + (k >= deadline ? n : 0);
    sale.total = dawn.own.shed[p] + carried;
    int first = 0;
    const int left = sale.solve(0, first);
    if (sold_today) *sold_today = sale.total - left;
    return sale.value;
}

// Day 29: the latest hour by which a product's output should reach the shed.
// Each candidate hour is valued by the liquidation DP; later hours are preferred
// unless they lose more than 1% of the best value.
int receipt_deadline(const agent::AgentObservation& dawn, const double rival[HOURS][N_PRODUCTS], int p, int shed,
                     int units, int hours, bool tie_now) {
    int demand[2 * HOURS]{};
    demand_by_step(dawn, p, hours, demand);
    ProductSale sale;
    sale.tie_now = tie_now;
    sale.product = p;
    sale.remaining = hours;
    sale.inv0 = dawn.market.inventory[p];
    sale.terminal = true;
    sale.rival_weight = 1;
    double net = 0;
    for (int k = 0; k < hours; ++k) {
        sale.rival_units[k] = int(std::lround(std::max(0.0, rival[k][p])));
        net += demand[k] - std::max(0.0, rival[k][p]);
        sale.cum_demand[k + 1] = int(std::lround(net));
    }
    const int candidates[] = {4, 8, 12, 16, 20, hours - 1};
    double values[6], best = -1e18;
    for (int c = 0; c < 6; ++c) {
        for (int k = 0; k < hours; ++k) sale.avail[k] = shed + (k >= candidates[c] ? units : 0);
        sale.total = shed + units;
        int first = 0;
        sale.solve(0, first);
        values[c] = sale.value;
        best = std::max(best, values[c]);
    }
    for (int c = 5; c >= 0; --c)
        if (values[c] >= best - std::max(1.0, 0.01 * std::abs(best))) return candidates[c];
    return hours - 1;
}
}

void choose_sales(const agent::AgentObservation& obs, const int incoming[HOURS][N_PRODUCTS], int hours,
                  double hold_discount, const int reserve[N_PRODUCTS], int night_room,
                  const double rival[HOURS][N_PRODUCTS], const int hold_supply[N_PRODUCTS], int sell[N_PRODUCTS],
                  bool tie_now, bool melon_tie_now) {
    std::fill_n(sell, N_PRODUCTS, 0);
    const int h = obs.hour;
    const int remaining = hours - h;  // market hours left today, including this one
    if (remaining <= 0) return;
    std::vector<ProductSale> sales;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        ProductSale sale;
        sale.tie_now = tie_now || (melon_tie_now && p == MELON);
        sale.product = p;
        sale.remaining = remaining;
        sale.inv0 = obs.market.inventory[p];
        sale.hold_discount = hold_discount;
        if (const char* d = std::getenv("DC10_HOUR_DISCOUNT")) sale.hour_discount = std::atof(d);
        // Held units are sold tomorrow after tomorrow's own output (about today's) and the
        // opponent supply expected before them.
        const char* own = std::getenv("DC10_OWN_SUPPLY");
        sale.hold_supply = hold_supply[p] + (own ? int(std::atof(own) * incoming[hours - 1][p]) : 0);
        sale.spot_hold = std::getenv("DC10_HOLD_SPOT") != nullptr;
        sale.terminal = obs.day == LAST_DAY;
        const int now_received = h > 0 ? incoming[h - 1][p] : 0;
        for (int k = 0; k < remaining; ++k)
            sale.avail[k] = std::max(0, obs.own.shed[p] + (incoming[h + k][p] - now_received) - reserve[p]);
        sale.total = sale.avail[remaining - 1];
        if (sale.total <= 0) continue;
        // Net market inventory removed before each step: demand minus expected opponent sales.
        int demand[2 * HOURS]{};
        demand_by_step(obs, p, remaining + HOURS / 2, demand);
        double net = 0;
        const char* weight = std::getenv("DC10_RIVAL_WEIGHT");
        const char* terminal_weight = std::getenv("DC10_TERMINAL_WEIGHT");
        sale.rival_weight = obs.day == LAST_DAY ? (terminal_weight ? std::atof(terminal_weight) : 0.0)
                                                : weight ? std::atof(weight) : 1.0;  // margin: opponent revenue counts against us
        for (int k = 0; k < remaining; ++k)
            sale.rival_units[k] = int(std::lround(std::max(0.0, rival[(obs.step + k) % HOURS][p])));
        for (int k = 0; k < remaining + HOURS / 2; ++k) {
            const double opponent = rival[(obs.step + k) % HOURS][p];
            net += demand[k] - (p == WHEAT || p == FERTILIZER ? opponent : std::max(0.0, opponent));
            sale.cum_demand[k + 1] = int(std::lround(net));
        }
        sales.push_back(sale);
    }
    if (const char* debug = std::getenv("DC10_SALE_DEBUG"); debug && obs.hour == std::atoi(debug))
        for (const auto& sale : sales) {
            std::fprintf(stderr, "sale p%d total %d inv %d price %d rival:", sale.product, sale.total, sale.inv0,
                         market_price(sale.product, sale.inv0));
            for (int k = 0; k < remaining; ++k) std::fprintf(stderr, " %d", sale.rival_units[k]);
            std::fprintf(stderr, " | net_demand_cum:");
            for (int k = 0; k <= remaining; k += 4) std::fprintf(stderr, " %d", sale.cum_demand[k]);
            std::fprintf(stderr, "\n");
        }
    if (const char* all = std::getenv("DC10_SELL_ALL"); all && obs.day >= std::atoi(all)) {  // diagnostic: sell all sellable stock now
        for (const auto& sale : sales) sell[sale.product] = sale.avail[0];
        return;
    }
    auto run = [&](double charge) {
        int left = 0;
        for (auto& sale : sales) left += sale.solve(charge, sell[sale.product]);
        return left;
    };
    if (run(0) <= night_room || night_room < 0 || obs.day == LAST_DAY) return;
    double low = 0, high = 1000;
    if (run(high) > night_room) return;  // even selling everything cannot make room
    for (int iteration = 0; iteration < 12; ++iteration) {
        const double mid = 0.5 * (low + high);
        (run(mid) <= night_room ? high : low) = mid;
    }
    run(high);
}
}
