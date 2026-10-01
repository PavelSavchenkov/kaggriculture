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
    // Price floor (MarketOptions::floor_sales): sales at $1 do not add market inventory (engine rule).
    // step_demand / future_rival: gross demand per step (today and 12 h of tomorrow) and the opponent's
    // expected sales in tomorrow's first 12 hours (future_floor: the floor applies there too).
    bool floor = false, future_floor = false;
    int step_demand[2 * HOURS]{};
    int future_rival[HOURS / 2]{};
    int solve(double charge, int& first);  // returns units left unsold; `first` = units sold now
    bool solve_floor(double charge, int& first, int& left);  // false: the floor is unreachable or the case too big
};

// Market inventory removed by town and shops at each of the next `steps` steps.
void demand_by_step(const agent::AgentObservation& obs, int item, int steps, int out[]);

// The day's market model, shared by the router's deposit values and the hourly seller: expected
// opponent sales (day 29: plus its visible output, liquidated over hours 1-12) and the value of
// stock held overnight (day 28: sold tomorrow after the opponent's visible output).
struct DayMarket {
    double rival[HOURS][N_PRODUCTS]{};
    int hold_supply[N_PRODUCTS]{};
    double hold_discount = 0.95;
    double rival_weight = 1;  // margin: weight of the opponent's revenue lost to our sales (day 29: 0)
    int grid_step = 2;        // deposit values: hours between exact evaluations (linear in between)
    bool interleave = false;  // ProductSale::interleave
    double wait_cost = 0;     // ProductSale::wait_cost
    bool wait_route_only = false;  // MarketOptions::wait_route_only
    int lot_cap = 0;               // MarketOptions::lot_cap
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
    // Sale DP: weight of the opponent's revenue lost to our sales (margin objective). 0.5: +755
    // (SE 366) over 200 fresh games vs 1 (frozen and top-team clones): with 1 the seller dumps at
    // midday to deny a forecast opponent flow that is often early or too large.
    double rival_weight = 0.5;
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
};
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
void choose_sales(const agent::AgentObservation& obs, const int incoming[HOURS][N_PRODUCTS], int hours, const DayMarket& market,
                  const int reserve[N_PRODUCTS], int night_room, int sell[N_PRODUCTS], double* charge_out = nullptr,
                  const double* cash_need = nullptr);

double sale_value(int item, int inventory, int units);
}
