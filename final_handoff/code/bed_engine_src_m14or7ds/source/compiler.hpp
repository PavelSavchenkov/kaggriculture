#pragma once
// Day compiler: DayIntent -> worker actions and market orders (designs/day_compiler.md).
// Worker routes come from the root day_policy solver; this file owns intent binding,
// purchases, returns, funding, sales and the fallback ladder.
#include "history.hpp"
#include "intent.hpp"
#include "day_policy_local/source/policy.hpp"

namespace dc10 {
namespace dp = kag::agents::day_policy_contract;

enum class CompileStatus { Ok, InvalidIntent, NoSchedule, Unfunded };
const char* status_name(CompileStatus s);

// DC10_PROFILE: prints the last compile_day's time per phase to stderr.
void print_profile(int day, double total_ms);

struct CompileOptions {
    dp::SolveOptions solve{};  // day_policy default: Balanced-4, hire minimization, cap 13
    bool collect_fertilizer = true;  // optional COLLECT_FERTILIZER on animals already visited
    double hold_discount = 0.95;     // value of a unit kept overnight, relative to tomorrow's price
    int cash_margin = 0;             // extra cash kept at every planned purchase
    bool return_output = true;       // request shed returns of harvested output
    bool water_for_production = true;  // water retained ongoing crops before doubled production
    // Time pressure (set per day by the agent from the remaining overage time):
    // 1 = fast search for the market-return attempt; 2 = also cap route executions.
    int time_pressure = 0;
    double time_left = 60;  // remaining overage (s), set per day by the agent: route_search needs > 40, race_dp > 30
    // Steady-clock time (ms, now_ms()) after which today's optional searches are skipped: race_dp and
    // race_steps return rungs here, herd/crop reach retries in the agent (0: no deadline). The agent
    // sets it at each dawn to 0.9 s plus an equal share of the remaining overage per remaining day.
    double deadline_ms = 0;
    // Agent configuration. Defaults: the tested best; the model's sidecar <model>.compiler and the
    // DC10_* variables of the same meaning override them (bc_opus Agent::reset), so each agent in a
    // process keeps its own settings.
    bool trim_model = false;    // over budget, drop the units the network is least likely to want (DC10_TRIM_MODEL)
    bool race = false;          // melon sale race (DC10_RACE)
    int race_deadline = 10;     // melon market-part receipts by this hour (DC10_RACE_DEADLINE)
    bool sale_tie_now = false;  // sale DP: equal value sells now (DC10_SALE_TIE_NOW)
    bool recovery = true;       // survival keeps the day's harvests and collections (DC10_NO_RECOVERY: off)
    int reserve_from_day = 1;   // next-dawn cash reserve from this day (DC10_RESERVE_FROM_DAY)
    // The engine fills both players' orders slot by slot, so a sale listed earlier than the
    // opponent's sale of the same product gets the higher prices. Sale order: 0 product index,
    // 1 descending price, 2 descending revenue at stake (price drop our quantity causes x units;
    // work/sep25_bc_weakness X13).
    int sell_order = 0;  // (DC10_SELL_ORDER)
    // Fertilize a crop only if the extra yield, at today's price, is worth at least the
    // fertilizer's market price (on wheat, carrots and melons it rarely is) (DC10_FERT_VALUE).
    bool fert_value = false;
    // Same-day market returns only if the sale DP's gain from them covers the extra wages of the
    // hires they add (Fibonacci: the 12th hire costs $144, the 13th $233) (DC10_RETURN_WAGES).
    bool return_wages = false;
    // Probe: feed and care every existing animal whenever today's care still banks a sold
    // production (each care+feed day adds 1 product) (DC10_FULL_CARE).
    bool full_care = false;
    int market_deadline = 20;  // hour by which the market part of returns reaches the shed (DC10_MARKET_DEADLINE)
    // Load leveling: wages grow like Fibonacci within a day, so a day that needs at least this many
    // hires is compiled again with the harvests and collections that lose nothing by waiting a day
    // moved to tomorrow, one hire fewer; kept if it compiles as well (0: off) (DC10_LEVEL_HIRES).
    int level_hires = 0;
    bool defer = false;  // internal: this compile defers the work that can wait
    // The pre-solve hire cap also counts dawn shed stock sellable at hour 0 (our sales precede our
    // hires in the order list); the hourly funding check still decides (DC10_HIRE_CAP_STOCK). Without
    // it, a dawn with little cash but a full shed could not route and fell to survival.
    bool hire_cap_stock = false;
    // Collect the fertilizer of every animal, also those with no other job today (an uncollected
    // unit is lost when the next one appears; work/sep25_bc_weakness "collectall") (DC10_COLLECT_ALL).
    bool collect_all = false;
    // Opponent sale forecast from its visible farm as well as its trailing sales: melons = its visible
    // ripe melons, other products half trailing and half visible supply, hourly shape from the trailing
    // flow (work/sep25_bc_weakness "blend", validated on 752 games) (DC10_FORECAST_BLEND).
    bool forecast_blend = false;
    int blend_visible = 50;  // forecast_blend: percent weight of visible supply for non-melon products (DC10_BLEND_VISIBLE)
    // forecast_blend: the opponent's supply also counts its inferred shed stock (observed harvests minus
    // inferred sales, History::opponent_stock), not only ripe and held output (DC10_FORECAST_STOCK).
    bool forecast_stock = false;
    // Adaptive opponent model: expected sales today = its estimated sell-through rate (last 3 days,
    // prior 0.6) x (inferred shed stock + ripe/held output), shaped by its trailing hourly flow;
    // replaces forecast_blend's fixed weights (DC10_FORECAST_ADAPT).
    bool forecast_adapt = false;
    // Fitted opponent model: expected sales today per product = linear model of its trailing sales,
    // visible ripe/held output, inferred shed stock, sell-through x supply and day (least absolute
    // deviation on 600 recorded games, tools/forecast_audit; held-out error 25-70% below blend);
    // hourly shape from the trailing flow; replaces forecast_blend (DC10_FORECAST_FIT).
    bool forecast_fit = false;
    // Online model selection: per product, the forecaster (blend or fitted) with the smaller absolute
    // error on this opponent's sales over the last 3 days (DC10_FORECAST_SELECT).
    bool forecast_select = false;
    // Condition the day's opponent forecast on its sales so far today: the remaining hours expect the
    // forecast total minus what it already sold, in the same hourly shape (DC10_FORECAST_INTRADAY).
    bool forecast_intraday = false;
    // Hourly shape of the forecast from all of this opponent's past sales of the product (the 3-day
    // trailing shape is often empty for products sold in waves, e.g. melons) (DC10_FORECAST_TIMING).
    bool forecast_timing = false;
    // The other session's anticipation stack (work/sep25_bc_weakness "oppprior_steps"): forecast_prior
    // shapes the melon/milk/wool forecast by a strong-player hourly prior (by day bucket, source/opp_prior.hpp),
    // half and half with the opponent's trailing shape when it has one; race_early lets race_dp deliver
    // from hour 6; race_steps retries an unroutable early delivery 1..5 steps of this many hours later;
    // melon_tie_now: equal sale value sells melons now (DC10_FORECAST_PRIOR, DC10_RACE_EARLY,
    // DC10_RACE_STEPS, DC10_MELON_TIE_NOW).
    bool forecast_prior = false, race_early = false, melon_tie_now = false;
    int race_steps = 0;
    int race_first = 6;  // race_early: earliest market-part delivery hour tried by race_dp (DC10 sidecar "race_first")
    // first_full (work/sep25_bc_weakness, Sep 26): an opponent that has not sold milk or wool in the trailing window
    // but holds visible output is forecast to sell all of it in the strong players' no-history hourly shape
    // (OPP_PRIOR, first wool: hours 3-7), not half of it (blend) or none (the funding check's trailing forecast).
    int first_full = 0;
    // race_stream_first / race_stream_rate (work/sep25_bc_weakness, Sep 26): an early (race) delivery of melons,
    // milk or wool is a cumulative stream: race_stream_first units by the early hour, then race_stream_rate more
    // per hour, instead of the whole early quantity by the early hour (one animal's or tile's yield first, as
    // strong players do; the whole quantity early costs extra hires).
    int race_stream_first = 0, race_stream_rate = 0;
    // ladder_effort (work/sep25_bc_weakness): solver effort of the market-return ladder attempts, 1 compact, 2 fast
    // (0: default); cuts the stream's retry cost (Sep 25: compact kept +4.3k of +4.7k at 2.4 s vs 4.1 s worst day).
    int ladder_effort = 0;
    // Wider route search (16 route variants, 8 workforce variants, opportunistic hire reduction): finds
    // routes with fewer hires (work/sep25_bc_weakness "routesearch", mirror +2.0k). 1: every solve;
    // 2: only when the day needs >= 12 hires (the 12th costs $144); 3: every solve with 8 route and
    // 4 workforce variants; off under time pressure (DC10_ROUTE_SEARCH).
    int route_search = 0;
    // 4: budgeted improvement pass (agent): after the day's plan is chosen, compile it again with the wide
    // search (mode 1) when the dawn's remaining budget covers ~3.5x the base compile time, and keep the
    // wide plan if it compiles at the same or a better level (reports/routeparts: wide +2.2k, 3.6x time).
    // Slack filling: when the day compiles in full, compile again feeding and caring for every animal
    // (each care+feed day banks +1 product) with the hire count capped at the plan's; kept if it still
    // compiles in full with no extra hire (full care alone forced hires: -1.3k) (DC10_SLACK_CARE).
    bool slack_care = false;
    // Market-part delivery hour per product chosen by the sale DP (10..20: the latest hour within 1% of
    // the best value), tried before the fixed deadline (work/sep25_bc_weakness "racedp") (DC10_RACE_DP).
    bool race_dp = false;
    // Capacity deferral: a plan whose night deposit (units still carried after the last hour) exceeds
    // the shed room left once the shed is sold down to its kept inputs loses the excess at night
    // (seed 1319 with a 4th quadrant: 91-203 units carried into the night, 2,691 units discarded in 20
    // games). Compile again with the harvests and collections that lose nothing by waiting moved to
    // tomorrow (day 28 crops to day 29) and keep it if it compiles at the same level with a smaller
    // night deposit (DC10_CAPACITY_DEFER).
    bool capacity_defer = false;
    // Value probe only, never set by a deployable agent (it is hidden future information, see
    // prompts/local_agent.md): the opponent's true sales per day, hour and product ([30][24][9], recorded
    // in a first pass of the same game by tools/search_games SEARCH_ORACLE) replace the forecast for crops
    // and animal products.
    const float* oracle_rival = nullptr;
    // The shed keeps tomorrow's first feed overnight (1 wheat per animal not in tonight's deposit): it
    // shrinks tonight's room for the harvest, and a night deposit beyond the room needs capacity
    // returns (42% of days, 11.9 hires vs 10.9 without returns). Off: the room is the whole shed and
    // tomorrow's feed wheat is bought at dawn (usable from hour 1) (DC10 sidecar "feed_reserve 0").
    bool feed_reserve = true;
    // Capacity triage: units carried into the night beyond the shed's room are discarded. When the
    // plan's night deposit overflows (4th-quadrant days: 125-167 carried, room 95, capacity returns
    // unroutable at 13 hires), drop the harvests of the cheapest crops for the overflowing units and
    // compile again: the labour goes to returning the valuable output instead (sidecar "capacity_trim").
    bool capacity_trim = false;
};

// CompileOptions::oracle_rival applied to a day's forecast.
void apply_oracle(const CompileOptions& options, int day, double rival[HOURS][N_PRODUCTS]);

// Fallback levels, in the order requirements are dropped (design: keep survival last).
enum Fallback : int { KeepAll = 0, NoLand, NoNewEntities, NoCollection, SurvivalOnly, FallbackLevels };

struct DayPlan {
    CompileStatus status = CompileStatus::NoSchedule;
    int fallback = KeepAll;  // level actually used
    int return_percent = 0;  // day 29: share of output returned; ordinary days: -1 market and
                             // capacity returns, -percent of the capacity part alone
    int funding = 0;         // 0: plain; 1-3 purchases first, 4-6 hires first, returns by hour 3/6/10
    bool stress_funded = false;  // purchases also covered under the stress forecast
    std::string reason;
    dp::DayInput input;
    Action actions[HOURS]{};  // units plus the solver's purchase and hire orders
    int receipts[HOURS][N_PRODUCTS]{};
    int night_carried = 0;  // units still carried after the last hour, deposited tonight
    int night_items[N_PRODUCTS]{};     // carried products deposited tonight, by product
    int keep_overnight[N_PRODUCTS]{};  // inputs kept for tomorrow's hour-0 pickups (wheat: at least 1 per animal)
    int hires = 0;
    int new_animals[N_ANIMALS]{};  // placed today (for the next-dawn cash reserve)
    int hours = HOURS;
    double compile_ms = 0;
    double shortfall = 0;  // cash missing in the first failed funding check (budget revision)
};

DayPlan compile_day(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent,
                    const CompileOptions& options, dp::Solver& solver);

double now_ms();  // steady clock, for CompileOptions::deadline_ms

// Follows a plan hour by hour and decides sales from the actual market.
class DayExecutor {
public:
    void start(const agent::AgentObservation& dawn, const History& history, const DayPlan& plan,
               const CompileOptions& options);
    void act(const agent::AgentObservation& obs, Action& action);
    const DayPlan& plan() const { return plan_; }
    void rebind(const History& history) { if (history_) history_ = &history; }  // after copying its owner
private:
    DayPlan plan_;
    CompileOptions options_;
    int day_ = 0;
    double rival_[HOURS][N_PRODUCTS]{};  // expected opponent net sales by hour of day
    int hold_supply_[N_PRODUCTS]{};       // day 28: opponent output sold before our held units
    double hold_discount_ = 0;
    const History* history_ = nullptr;  // live history: opponent stock is re-read every hour
};

// Sales for one hour: how many units of each finished product to sell now.
void choose_sales(const agent::AgentObservation& obs, const int incoming[HOURS][N_PRODUCTS], int hours,
                  double hold_discount, const int reserve[N_PRODUCTS], int night_room,
                  const double rival[HOURS][N_PRODUCTS], const int hold_supply[N_PRODUCTS], int sell[N_PRODUCTS],
                  bool tie_now = false, bool melon_tie_now = false);
}
