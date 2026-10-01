#pragma once
// Market side of the day compiler: opponent sale forecast, sale DP, deposit values.
// Behaviour of the frozen agent's configuration (fin_Z_rs_l3: forecast_blend + forecast_select,
// melon ties sell now, hold discount 0.95), without the dead variants.
#include "source/history.hpp"
#include <functional>
#include <vector>

namespace dc11 {
using namespace dc10;

// Expected opponent net sales by hour of day and product (negative: purchases). Per product, the
// trailing 3-day flow reshaped to either a blend of trailing and visible supply or a fitted model,
// whichever predicted this opponent better over the last 3 days.
// first_full: see first_sales.
void forecast(const agent::AgentObservation& dawn, const History& history, double rival[HOURS][N_PRODUCTS], int first_full,
              double blend = 0.5);

// First milk or wool days (work/sep25_bc_weakness "first_full"): a product the opponent has not sold
// in the trailing flow but holds as visible output is forecast as all of that output, sold in the
// strong players' no-history hourly shape (OPP_PRIOR; first wool mostly at hours 3-7).
void first_sales(const agent::AgentObservation& dawn, double rival[HOURS][N_PRODUCTS]);

// Stress forecast (funding safety): the expected flow plus all visible opponent output at `hour`.
void stress_forecast(const agent::AgentObservation& dawn, const History& history, double rival[HOURS][N_PRODUCTS], int hour = 2);

// Sale plan for one product over today's remaining markets (dynamic programming over units sold
// per hour), with the opponent's expected sales after ours and a hold value for unsold units.
struct ProductSale {
    int product = 0, total = 0, remaining = 0, inv0 = 0;
    int avail[HOURS]{};
    int cum_demand[2 * HOURS + 1]{};
    int rival_units[HOURS]{};
    double rival_weight = 1;   // margin: opponent revenue counts against us (0 on day 29)
    // rivalnight (dc12): weight of the opponent's forecast revenue from tonight to the hold step (future_rival). A unit we sell
    // today stays in the book until drained, so it also lowers the price of the opponent's sales tonight and tomorrow morning; a
    // unit held to the hold step does not. The same-day rival term stops at the day's last market. 0: off.
    double rival_night = 0;
    double night_cost(int s) const;  // rival_night x the opponent's revenue over the hold horizon with s units sold today
    double hold_discount = 0.95;
    int hold_supply = 0;       // units reaching the market before held units are sold tomorrow
    bool terminal = false;
    bool tie_now = false;
    // Same-turn sales as the engine runs them: our i-th unit and the opponent's i-th unit get the same
    // quote, so both pay for each other's earlier units (off: ours first, the opponent's after).
    bool interleave = false;
    double wait_cost = 0;      // each hour of waiting keeps (1 - wait_cost) of the later value (0: off)
    bool keep_first = false;   // record first_values (scenario seller)
    int lot_cap = 0;           // lotcap: at most this many units per hour, except in the last hour (0: off)
    std::vector<double> first_values;  // value of selling q = 0..avail[0] units now, the plan after it optimal
    double value = 0;
    double bonus[HOURS]{};    // cashsell: extra value per dollar received at step k (cash the plan's purchases need by then)
    double revenue[HOURS]{};  // the solved schedule's revenue per step
    int plan_lots[HOURS]{};   // the solved schedule's units per step
    // Price floor (MarketOptions::floor_sales): sales at $1 do not add market inventory (engine rule).
    // step_demand / future_rival: gross demand per step (today and 12 h of tomorrow) and the opponent's
    // expected sales in tomorrow's first 12 hours (future_floor: the floor applies there too).
    bool floor = false, future_floor = false;
    int step_demand[2 * HOURS]{};
    int future_rival[HOURS]{};
    int hold_steps = HOURS / 2;  // holdsteps: steps into tomorrow at which units held tonight are priced (the lot's reference hour)
    // dc12 response (MarketOptions::response): the opponent's expected units at a later step scale with the price level our own
    // sales leave (level = inventory - level_ref, the last day's mean inventory), quasi-statically in our units sold s.
    bool response = false;
    double level_ref = 0;
    double base_rival[2 * HOURS]{};  // the forecast's opponent units per step (response base)
    std::vector<int> cum_s, rival_s;  // per s: cumulative net demand to each step, and the opponent's units per step
    void build_response();
    // dc12 scenopen: the mean-path DP's policy (units to sell at step k from s sold), and the value of a fixed lot schedule on
    // this sale's path with the DP's own accounting (own revenue, rival cost, hold value), for non-anticipative scenario scoring.
    bool keep_policy = false;
    std::vector<int> policy;
    double evaluate(const int* lots) const;
    int inventory_at(int k, int s) const { return response ? inv0 + s - cum_s[size_t(s) * (remaining + HOURS / 2 + 1) + k] : inv0 + s - cum_demand[k]; }
    int rival_at(int k, int s) const { return response ? rival_s[size_t(s) * remaining + k] : rival_units[k]; }
    int solve(double charge, int& first);  // returns units left unsold; `first` = units sold now
    bool solve_floor(double charge, int& first, int& left);  // false: the floor is unreachable or the case too big
};

// dc12 fieldflow: the opponent's thin-product forecast (strawberry, egg, milk, wool) replaced by f x each drain, sold in the hour
// after it (the field takes the drained room: live top teams sell ~0.6-0.75x the drain after each tick, Imitation). Days < 29.
void field_flow(const agent::AgentObservation& obs, double f, double rival[HOURS][N_PRODUCTS]);

// Market inventory removed by town and shops at each of the next `steps` steps.
void demand_by_step(const agent::AgentObservation& obs, int item, int steps, int out[]);
// dc12 stockcap: scale the opponent's forecast sales of hours [from, to) down to what it holds (inferred stock + visible output).
void cap_by_stock(const agent::AgentObservation& obs, const History& history, double rival[HOURS][N_PRODUCTS], int from, int to);

// The day's market model, shared by the router's deposit values and the hourly seller: expected
// opponent sales (day 29: plus its visible output, liquidated over hours 1-12) and the value of
// stock held overnight (day 28: sold tomorrow after the opponent's visible output).
struct DayMarket {
    double rival[HOURS][N_PRODUCTS]{};
    int hold_supply[N_PRODUCTS]{};
    double hold_discount = 0.95;
    int hold_steps = HOURS / 2;  // MarketOptions::hold_steps
    double rival_weight = 1;  // margin: weight of the opponent's revenue lost to our sales (day 29: 0)
    double rival_night = 0;   // MarketOptions::rival_night
    double tick_share = 0, tick_dawn = 0, tick_egg = 0;  // MarketOptions::tick_share / tick_dawn / tick_egg
    bool no_fire = false;     // Options::no_fire (set by the executor): an uncoverable purchase gets no cashsell bonus
    int grid_step = 2;        // deposit values: hours between exact evaluations (linear in between)
    bool interleave = false;  // ProductSale::interleave
    double wait_cost = 0;     // ProductSale::wait_cost
    bool wait_route_only = false;  // MarketOptions::wait_route_only
    int lot_cap = 0;               // MarketOptions::lot_cap
    bool response = false;         // MarketOptions::response
    bool lumpy = false;            // MarketOptions::lumpy
    double lump_q[HOURS][N_PRODUCTS]{};  // lumpy: the opponent's selling frequency by hour (set by the executor)
    bool scen_open = false;        // MarketOptions::scen_open
    // dc12 seller (MarketOptions::race): per product, the opponent's reservation level (the inventory the market kept) and
    // its available stock; set by the executor each hour.
    bool race = false;
    double leader = 0;             // MarketOptions::leader
    double race_level[N_PRODUCTS]{};
    int race_stock[N_PRODUCTS]{};  // leader: the opponent's inferred stock now (race_opp - its visible output)
    double own_hist[HOURS][N_PRODUCTS]{};  // leader: our net sales per hour of day over the last 3 days (the opponent's forecast of us)
    int race_opp[N_PRODUCTS]{};
    bool tie_all = false;     // ties between selling now and later sell now for every product (else melons only)
    int dawn_hours = 0, dawn_start = 0;  // MarketOptions::dawn_sell / dawn_start on days 1-28
    int scenarios = 0;        // MarketOptions::scenarios
    bool carry[N_PRODUCTS]{};  // regime M: carried products (the executor sets them)
    bool dawn[N_PRODUCTS]{};   // regime M: products sold dawn-first
    double carry_wait = 0;     // regime M: the seller's wait cost per hour for dawn products
    double carry_hold = 0;     // regime M: the seller's hold discount for carried products (0: hold_discount)
    bool floor_sales = false, future_floor = false;  // MarketOptions::floor_sales / future_floor (days 6-28)
};
struct MarketOptions {
    double hold_discount = 0.95;     // value of a unit kept overnight relative to tomorrow's price
    // holdsteps=k (dc12): units kept overnight are priced as one lot k steps into tomorrow (default 12: after the h0 / h4 / h8 drains).
    // In a linear market with drains and a fixed opponent path one late lot is optimal; at 12 the afternoon drains are never counted,
    // so a production pulse looks cheaper to sell tonight (Imitation: half of M&M's price lead is which days it sells on).
    int hold_steps = HOURS / 2;
    // Sale DP: weight of the opponent's revenue lost to our sales (margin objective). 0.5: +755
    // (SE 366) over 200 fresh games vs 1 (frozen and top-team clones): with 1 the seller dumps at
    // midday to deny a forecast opponent flow that is often early or too large.
    double rival_weight = 0.5;
    // rivalnight=W (dc12): the margin term also counts the opponent's forecast sales from tonight to the hold step (ProductSale::
    // rival_night). 0: off.
    double rival_night = 0;
    // fieldflow=f (dc12): the thin-product opponent sells f x each drain in the hour after it (field_flow) instead of the forecast.
    // At f = 1 the book is flat and waiting for drains gains nothing. 0: off.
    double field_flow = 0;
    // ticksell=a tickdawn=b (dc12 probe, M&M's selling rule copied, not a model): strawberry / milk / wool are sold only at h0 (share b
    // of the stock) and in the hour after each shop drain (h % 4 == 1: share a), nothing in between; the night room's forced sales
    // and the day's last market stay the DP's. M&M (same-stock bed, days 12-24): ~1 unit per product per tick (about a third of the
    // drain, more with stock, less when the book is above the day's mean), a dawn lot of ~1/4 of the overnight strawberries.
    double tick_share = 0, tick_dawn = 0;
    double tick_egg = 0;  // tickegg=f (probe): eggs sold only at h1 (0.3 of stock) and h21-22 (share f): M&M sells 53% of eggs at h21-23
    // first_sales in the forecast (0: off). 1 with Options::first_funding off:
    // positive margins on all tested beds (v28).
    int first_full = 1;
    int grid_step = 2;               // deposit values: hours between exact evaluations
    double blend = 0.5;              // forecast blend: weight of the opponent's visible supply
    // Same-turn sales priced as the engine runs them (ProductSale::interleave): dc11 zoo +4.6k, wins
    // 576 -> 687 of 720; vs frozen +1.6k (SE 0.6k); Q4 bed +3.4k (v29).
    bool interleave = true;
    // Ties between selling now and later sell now for every product (v32). With integer prices and
    // a smoothed forecast ties are common; keeping "later" deferred milk/wool to the evening lot. Real
    // pinned Kaggle games: +0.86k (503 top-LB) / +0.91k (338 ours), positive at every rank.
    bool tie_all = true;
    double wait_cost = 0;            // ProductSale::wait_cost: a small risk premium on later sales
    bool wait_route_only = false;    // wait_cost only in deposit values (router), not in the hourly seller
    // lotcap=N: the sale DP plans at most N units of a product per hour (not wheat or fertilizer; the last hour is uncapped, where
    // the night room decides), so lots follow the top teams' small sizes (Weaknesses' maxlot, applied inside the plan).
    int lot_cap = 0;
    // race=1 (dc12 seller, sep29_dc12/DESIGN.md): the opponent is a reservation-level seller that sells back up to the level the
    // market kept whenever drains push the inventory below it. We sell first at each post-drain hour (h1, h5, ..., h21) the units
    // that bring the inventory back to that level, plus what our stock and today's incoming output cannot fit into the drains
    // until tomorrow's first post-drain hour; otherwise we hold. Strawberries, milk, wool, fertilizer; the DP decides the rest,
    // the last market hour (night room), and products the opponent has no stock of.
    bool race = false;
    // leader=1 (dc12 seller, sep29_dc12/DESIGN.md "leader"): our DP is a follower (a best response to a forecast of the other
    // side's schedule) and can never take a first mover's gain; in the mirror two followers wait into the same evening. Per thin
    // product the opponent holds, a Stackelberg choice between the DP's plan and "sell down to inventory level l" schedules
    // (l around the last day's mean inventory; units above the level sell right after each drain), each scored on the joint
    // path where the opponent, holding its inferred stock (its visible output arriving over the day), re-plans every hour with the
    // same DP on the actual inventory and a forecast of OUR flow from our last 3 days (it cannot see our plan; v0 let it see our
    // whole schedule, it front-ran every committed lot and the model held more: rung 1 -141 / day). The opponent's type is unknown (top teams commit to a schedule, our lineage follows): leader=w scores each
    // schedule as w x the follower path + (1 - w) x the DP's own value on the fixed forecast.
    double leader = 0;
    // response=1 (dc12 seller, sep29_dc12/DESIGN.md): the opponent's forecast sales respond to the price level our own sales leave
    // (a U-shaped chance to sell by level: it sells more when the price is well above normal and when the market is breaking).
    bool response = false;
    // stockcap=1 (dc12, Imitation's forecast audit): the opponent's forecast sales over the rest of the day are capped by what it
    // can sell: its inferred shed stock plus the output visible on its board (ripe crops, product on animals); zero when it holds none.
    bool stock_cap = false;
    // scenopen=1 (dc12, Imitation's audit): the scenario seller scores each lot for this hour with ONE continuation policy (the
    // mean-path DP's) on every sampled path, instead of each path's own hindsight-optimal continuation, which overvalued waiting
    // every hour (information-relaxation bias) and rolled the lots into the evening.
    bool scen_open = false;
    // lumpy=1 (dc12): scenario paths sample the opponent's lot at each hour as a lump: it sells with its historical frequency at that
    // hour (last 5 days) a lot of forecast / frequency (mean kept), instead of Poisson around the mean (top teams' dawn lots are
    // 4-7 units on 15-25% of days).
    bool lumpy = false;
    // Scenario seller (kaggriculture-38): the hourly seller chooses this hour's units by their mean value
    // over K opponent sale paths sampled from the forecast (Poisson per hour), each followed by the plan
    // optimal for that path; 0: the mean flow alone. v34: K = 16. Real pinned games (learned stack)
    // top-LB +0.30k [+0.12, +0.48], own money up; DSM's recorded games +0.58k; gate vs v32 paired P1
    // +0.9k, P3 +0.2k; no negative bed.
    int scenarios = 16;
    // Hourly seller (days 6-28, carrots to wool): sales at $1 do not add market inventory (engine rule), so
    // a market at the floor stops falling; future_floor also caps tomorrow's first 12 hours in the hold
    // value (ST5, Local-LB PR 214). Removed in v38 (closed on E), restored Sep 28 with the teammate's econ package
    // (alone ~0 on v17main; with collectmin round-robin +0.59k).
    bool floor_sales = false, future_floor = false;
    // Learned opponent forecast (BC session): replaces the day market's flow after forecast().
    std::function<void(const agent::AgentObservation&, const History&, double (*)[N_PRODUCTS])> learned;
    // Intra-day re-forecast (BC session): the executor calls it every hour on its copy of the day market.
    std::function<void(const agent::AgentObservation&, double (*)[N_PRODUCTS])> intraday;
    // Carry target (BC's <model>.carry sidecar, regime M): predicted next-dawn shed stock per product (units >= 0) as the top teams
    // keep it; set by the agent, called on the dawn observation with its History. Unset: no target.
    std::function<void(const agent::AgentObservation&, const History&, double*)> carry_target;
    // local addition (nextm): adds the learned next-morning opponent supply correction to hold_supply (days < 28)
    std::function<void(const agent::AgentObservation&, const double (*)[N_PRODUCTS], int*)> next_morning;
    // Test opponents (Weaknesses session): on days 1-28 sell all shed stock above reserves in hours
    // dawn_start .. dawn_start + dawn_sell - 1 and otherwise hold unless tonight's shed room forces sales.
    int dawn_sell = 0, dawn_start = 0;
    const double* oracle = nullptr;  // probe only (tools/oracle): the true flow [day][hour][product]
    // Probe only (DC12_ORACLE_MODE with the oracle flow): thin products' remaining flow from hour h mixes the forecast with the
    // truth. 1: true volume, forecast hourly shape; 2: forecast volume, true hourly shape; 3: the true flow (thin products only).
    const double* oracle_mix = nullptr;
    int oracle_mode = 0;
    unsigned oracle_products = ~0u;  // DC12_ORACLE_PRODUCTS: bitmask of the products mixed
    int oracle_first = 0, oracle_last = HOURS - 1;  // DC12_ORACLE_HOURS=a-b: mode 3 uses the true flow only for hours a..b
};
void mix_oracle(const MarketOptions& options, int day, int from, double rival[HOURS][N_PRODUCTS]);
DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options);

// Value (sales today + held stock) when `n` of `carried` units reach the shed at `hour`, the rest
// tonight; sold_today: units the DP sells today.
double return_value(const agent::AgentObservation& dawn, const DayMarket& market, int p, int carried, int n, int hour, int hours,
                    int* sold_today = nullptr);

// Per-unit value of depositing today's output of each product at each hour, relative to carrying
// it overnight (day 29: the unit's liquidation value, nothing if carried).
void deposit_values(const agent::AgentObservation& dawn, const DayMarket& market, const int output[N_PRODUCTS], int hours,
                    double gain[N_PRODUCTS][HOURS]);

// The opponent's positive flow from hour `from` on, moved into one `length`-hour block (0: starting
// 1 hour later, 1: 7 hours later, 2: 14 hours later; + shift): its sale hours are uncertain.
void timing_scenario(const double rival[HOURS][N_PRODUCTS], int from, int hours, int block, double out[HOURS][N_PRODUCTS],
                     int shift = 0, int length = 7);

// Sales for one hour: units of each product to sell now.
// cash_need[k] (cashsell; nullptr: off): dollars still missing, before this hour's sales, for the planned purchases up to step k;
// sales up to the first uncovered step get a revenue bonus, the smallest that covers it (or the largest tried).
extern thread_local bool g_sell_quiet;  // DC11_AUDIT (local, Imitation): true while the compiler simulates the executor
void choose_sales(const agent::AgentObservation& obs, const int incoming[HOURS][N_PRODUCTS], int hours, const DayMarket& market,
                  const int reserve[N_PRODUCTS], int night_room, int sell[N_PRODUCTS], double* charge_out = nullptr,
                  const double* cash_need = nullptr, const bool* force = nullptr);
// force[p] (sellmodel=3): product p sells this hour; the lot is the DP's best of at least one unit.
// Probes: true while the agent's live executor acts (not the compiler's internal executor simulations).
extern thread_local bool live_seller;

double sale_value(int item, int inventory, int units);
}
