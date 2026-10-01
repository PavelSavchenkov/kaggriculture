#pragma once
// Day compiler dc11 (DESIGN.md): DayIntent -> worker actions and market orders.
// One pass: bind the intent to stops and new entities, value deposits with the sale DP, route
// (joint placement and hire count), write the actions and just-in-time purchases, verify them in
// the engine, check funding with the hourly executor; unaffordable purchases become release
// times, then new entities are trimmed.
#include "dc11/market.hpp"
#include "dc11/router.hpp"
#include "source/intent.hpp"
#include <chrono>
#include <string>

namespace dc11 {
enum class CompileStatus { Ok, InvalidIntent, NoSchedule, Unfunded };
const char* status_name(CompileStatus s);
// Requirement classes given up, in order (design: survival last).
enum Fallback : int { KeepAll = 0, NoLand, NoNewEntities, NoCollection, SurvivalOnly };

struct Options {
    MarketOptions market;         // forecast and sale DP settings (market.hpp)
    int max_hires = 13;
    double wage_per_turn = 2;     // router: value of a worker turn
    // Work budget per compile (router route evaluations, ~3,700 per ms; deterministic): past it no
    // further funding variant or trim is tried.
    long max_evaluations = 5000000;
    // Soft deadline (the caller's DecisionBudget): checked where the work budget is.
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::time_point::max();
    bool spent(long evaluations) const {
        return evaluations > max_evaluations || std::chrono::steady_clock::now() >= deadline;
    }
    // Weight of the deposit values averaged over opponent sale-timing scenarios; the rest uses the
    // forecast's own hour profile (1: +1.43k vs 0 over 80 games with the old pricing). With
    // same-turn pricing, 0.5 wins on every reactive bed (frozen +3.3k, SE 0.6k; clones +1.1-2.1k)
    // and on the Local-LB roster (+1.4k / 254 games) but loses on pinned replays of real games
    // against top-30 teams (-3.5k / 32, -2.6k / 11). Default 1 (v29, shipping until a Kaggle A/B);
    // snapshots/v30 has 0.5.
    double timing_mix = 1;
    int timing_shift = 0, timing_length = 7;  // scenario blocks start at hours 1, 7, 14 (+ shift)
    bool final_return_always = true;  // false: automatic end-of-day deposit trips only on the last day
    double return_cost = 1;           // end-of-day deposit trip taken when its gain exceeds this x its turns
    int stress_hour = 2;  // funding safety: all visible opponent output sold at this hour
    bool first_funding = false;  // first_full also in the expected-forecast funding check (delays investment)
    int search_radius = 4, search_rounds = 6, site_candidates = 5;  // router neighbourhoods (Problem)
    // dropany=K: the hire search may also dissolve one of the K least-loaded hires' routes, not only the last (not on land days)
    int drop_any = 0;
    // Router neighbourhoods on farms with 4 quadrants at dawn, when > 0 (quad_radius / quad_rounds): Q4 bed +0.4k / +1.1k on two
    // seed sets at 8 / 12 (13-hire days 46% -> 38%); everywhere, 8 / 12 lost 0.26k on the dc11 zoo (Sep 26).
    int q4_radius = 0, q4_rounds = 0;
    double site_scale = 1;  // long-term site cost weights of new crops and animals (x this)
    // The agent gives dc11 the history caught up to the dawn observation (yesterday's hour 23 is
    // otherwise inferred only after the dawn compile; the network keeps its trained view).
    bool fresh_history = false;
    // Plan menu (st4 idea, days 6-28): alternatives with earlier deliveries (router-only time preference;
    // bit 1: 0.998, bit 2: 0.995), each played against the forecast with the live seller; the best day
    // value wins (our money + held stock - rival_weight x the opponent's money). 0: off.
    // slotrival=L: sale slots also weigh the opponent's expected units this hour (L x our units x their price drop), so products
    // both players sell this step trade before theirs (the k-th orders of both players execute together); 0: our price impact only.
    double slot_rival = 0;
    // cashsell: the seller covers the plan's purchases in time (a revenue bonus on sales up to the first unfunded one) instead of
    // selling only at the purchase hour; the funding check plays the same seller.
    bool cash_sell = false;
    // wheatcash: funding variants that sell the dawn shed wheat at hour 0 (feed from today's harvests or wheat bought back from
    // the variant's hour), tried after the same hour's plain variant.
    bool wheat_cash = false;
    // lazyfeed=1 (Sep 29, v93): from a dawn whose cash and stock cannot pay the first feed-wheat purchase (M&M ends day 0 at $0
    // on purpose), the funding deferral variants fetch the wheat at the first feed, after the trips that collect and sell
    // fertilizer; without it dc11 compiled an empty day there and the animals starved (5 of 52 M&M dawn-1 states, +1.9 to
    // +3.5k by day 10). Our own games reach such dawns too (day 3): pinned -0.37k trimmed [-0.86k, +0.04k], so off by default.
    bool lazy_feed = false;
    // reserve=0 (Sep 29, v95): no next-dawn reserve in the funding check (end assets >= animals x wheat price x 1.2 + 30). With
    // cashsell the dawn sale finances tomorrow's feed; the reserve made the day-6 wave trim a cow and end with cash unspent.
    bool dawn_reserve = true;
    // saleslots=3 (dc12 m1, Sep 29; experiments/v10/sep29_dc12): the engine takes 10 orders a turn, one per hire, and dc11 filled the
    // first hour with hires, so the seller's dawn lots were dropped. The seller's dawn decision (choose_sales at h0 on the dawn stock)
    // names the thin products it sells; they take first-hour slots from hires only when routing again with the smaller first wave
    // keeps the crew and drops no required stop. Pinned real games +362 (SE 109); with the other m1 keys +1,495 trimmed.
    int sale_slots = 0;
    // nighttrim=1 (dc12 m2, Sep 29): when tonight's cargo still exceeds the shed room after the router's deposit repairs, remove pure
    // output stops (harvest / collect only) until it fits; their product stays on the animal / crop. Full games vs our clones lost
    // ~10-12 units per game at the night deposit (end-game nights: 143-148 units in pockets vs capacity 100). Contested bed +268 (SE 129).
    bool night_trim = false;
    // survivalfloor=1 (dc12 m3, Sep 29): when every level is unfunded, the SurvivalOnly level's routed plan is executed anyway instead
    // of an idle day (idle: the animals go unfed and escape after two days; seat-swap game 114911253: $10 at dawn day 9 -> 13
    // escapes, -186k; with the key -7.9k).
    bool survival_floor = false;
    // regime=1 (the top teams' way of running the day; experiments/v10/sep28_top_lb_imitation/REDESIGN.md; off by default):
    // - products in regimecarry (bit mask over products; default strawberries, eggs, milk, wool = 232) earn no deposit value
    //   before regimeeve (18): no morning deposit trips, they ride in pockets to an evening shed visit or the day-end deposit;
    // - shed stops PLACE each product, so collected fertilizer and picked-up supplies stay carried (no drop + pickup loop);
    // - capacity: while tonight's carry home exceeds regimelimit, the day is recompiled with one carried product fewer (in the
    //   value order wool, milk, strawberry, tomato, egg); regimeroom keeps room for the other goods carried home;
    // - regimemorning=c: route cost c per hour an animal stop starts after hour 11 (the morning animal block; 1 in tests);
    // - no plan menu;
    // - seller (opt-in, a live A/B question): hold carried products overnight at regimehold x tomorrow's value, sell them
    //   dawn-first (regimewait per hour for the regimedawn mask: milk, wool, fertilizer; eggs stay evening sellers);
    //   regimepockets=1 counts tonight's pockets as tomorrow's supply in the hold value.
    int regime = 0;
    int regime_carry = (1 << STRAWBERRY) | (1 << EGG) | (1 << MILK) | (1 << WOOL);
    int regime_eve = 18;
    int regime_limit = 90;
    int regime_skip = 0;       // internal: carried products dropped from the end of the value order
    int regime_room = 25;      // regimeroom=R: units of tonight's room kept for other goods carried home (the carry budget is the rest)
    double regime_wait = 0;    // regimewait=w: the seller's wait cost per hour for regimedawn products (dawn-first); 0: off
    int regime_dawn = (1 << MILK) | (1 << WOOL) | (1 << FERTILIZER);  // regimedawn=<mask>: products sold dawn-first (default 448)
    double regime_hold = 0;    // regimehold=d: the seller's hold discount for carried products; 0: the market's (default)
    bool regime_pockets = false;
    double regime_morning = 0;  // regimemorning=c: route cost per hour an animal stop starts after hour 11 (0: off)
    int regime_products() const { return __builtin_popcount(unsigned(regime_carry) & ((1u << N_PRODUCTS) - 1)); }
    int plan_menu = 0;
    // Router night-cargo repair (4-quadrant farms carried 120-155 units into a 100-slot shed): rounds that
    // raise deposit gains before the last market until the cargo fits (v37 default 30; nightfix=3 is v36's).
    int night_rounds = 30;
    int race_lot = 0;  // test opponents: milk and wool sold only right after shop purchases, up to this per turn
    // Collect every animal at its held cap that produces tonight (its product would be lost): Q4 bed
    // +350 (SE 166), neutral where caps do not occur (v29).
    bool cap_collect = true;
    double collect_min = 0;  // collectmin=P: skip fertilizer collections below price P unless today's fertilizing needs them
    // startcomplete=N: plans of days 0..N-1 that drop stops are trimmed until complete (starved-opening guard; default 1, 0: off)
    int day0_complete = 1;
    // feednext=D: on days 1..D the executor keeps no overnight wheat reserve: own wheat can be sold for today's purchases and the
    // next dawn's plan buys the feed (wheat prices are flat early; the top teams sell their early harvest and buy feed). 0: off.
    int feed_next = 0;
    // prodmin=F: an animal product (egg, milk, wool) is not collected while its price is below F x its base price (days before 28);
    // it waits on the animal (production past the held cap is lost), which keeps a saturated market from being fed further
    // (e.g. wool without a Yarn Store). 0: off.
    double product_min = 0;
    // feedlate=D: on days 1..D tomorrow's first feed is bought at the last market hour instead of kept from shed stock all day,
    // so the seller can turn that wheat into cash for today's purchases (0: off)
    int feed_late = 0;
    // defercollect=1: an animal-product collection whose product can wait on the animal (held + tonight's production within the
    // held cap, days before 28) is optional work worth the hold discount it saves, so peak days can leave it for tomorrow
    // instead of hiring for it (0: off)
    bool defer_collect = false;
    // optlate=1: router lateness and the hire search's completeness count required work only (Problem::optional_late)
    bool optional_late = false;
    // bandsell=1: a price-responsive rule seller for strawberries, eggs, milk and wool (the top teams' sale curves, Weaknesses'
    // q286): each hour sell `rate` of the product's shed stock (fractions carry to later hours), rate = r while the market
    // inventory is at most a units above its base level, falling linearly to s at b above it; lots of at most `lot`. The
    // plan's last hour and day 29 stay with the sale DP (the night room and the liquidation decide there). r < 0: the DP sells
    // that product. Keys: bandsell, bandlot, and per product <straw|egg|milk|wool><a|b|r|s>.
    struct BandCurve { double a = 0, b = 0, r = -1, s = 0; };
    bool band_sell = false;
    // respsell=k (test opponents only; Weaknesses' price-selective clone seller for the F2R bed): strawberries, milk and wool are
    // sold only while the hour-start price is at least k x a trailing mean of hour-start prices (an EMA with weight respalpha per
    // hour, kept across days), up to resplot units per hour. The plan's last hour and day 29 stay with the sale DP; while
    // tonight's room binds, the DP's sale is the floor. 0: off.
    double resp_sell = 0;
    int resp_lot = 4;
    double resp_alpha = 0.05;
    bool resp_filter = false;  // respfilter=1: keep the DP's sale (capped at resplot) when the price passes, else hold
    bool band_room = false;  // bandroom=1: while tonight's room binds, sell at least what the DP sells this hour (it clears the room)
    int band_lot = 4;
    BandCurve band[N_PRODUCTS] = {{}, {}, {}, {-10, 50, 0.09, 0.012}, {}, {}, {5, 62, 0.21, 0.03}, {40, 58, 0.095, 0.012}, {}};
    // landtrim (Weaknesses session): an unfunded land day trims up to this many new entities with the
    // land kept before the NoLand fallback (which drops the land and every entity on it).
    int land_trims = 0;
    // cropfirst=N: on days 0..N-1 trims drop new crops before new animals (default 1: day 0 only), so an early cow survives the
    // funding trims at the cost of seeds (Imitation's day-3 cow; the copy-only probe cropfirst=5 on v17main: +89, SE 326).
    int crop_first = 1;
    // trimnet=1: trims drop the network's least-wanted unit first (DayIntent::trim_order, cursor trim_next), then the cost order
    // (sep24_BC_opus local addition, step 28; carried here so its vendoring skips the patch).
    bool trim_net = false;
};

struct Plan {
    CompileStatus status = CompileStatus::NoSchedule;
    int fallback = KeepAll;
    bool stress_funded = false;
    std::string reason;
    int hours = HOURS;
    Action actions[HOURS]{};           // unit actions + purchase, hire and land orders
    int receipts[HOURS][N_PRODUCTS]{}; // cumulative deposits by the end of each hour's worker phase
    int night_carried = 0;             // units carried after the last hour (deposited tonight)
    int night_items[N_PRODUCTS]{};
    int hires = 0;
    int dawn_wheat_sale = 0;           // wheatcash: dawn shed wheat sold at hour 0
    int new_animals[N_ANIMALS]{};      // placed today
    int dropped = 0;                   // intent stops or entities not served
    int skipped = 0;                   // optional work not done (extra fertilizer collections)
    int collects = 0, collects_dropped = 0;  // diagnostics: animal product collections bound, not served
    int visits = 0, tiles_worked = 0, revisits = 0;  // diagnostics: tile visits, tiles, tiles visited twice or more
    int trims = 0;
    int choice = 0;                    // plan menu: chosen alternative (0: the base plan)
    int moves = 0, unit_actions = 0, waits = 0;
    double compile_ms = 0;
    long evaluations = 0;
    double ms_route = 0, ms_realize = 0, ms_fund = 0;  // profile
};

Plan compile_day(const agent::AgentObservation& dawn, const History& history, const DayIntent& intent,
                 const Options& options);

// Follows a plan hour by hour; sales, purchases and hires from the actual market. `history`: the
// observed history (day 29: the opponent's remaining stock).
class Executor {
public:
    void start(const agent::AgentObservation& dawn, const History& history, const Plan& plan, const Options& options);
    void act(const agent::AgentObservation& obs, const History& history, Action& action);
    const Plan& plan() const { return plan_; }
private:
    Plan plan_;
    Options options_;
    int day_ = -1;
    DayMarket market_;
    double band_acc_[N_PRODUCTS]{};  // bandsell: fractional units carried to the next hour
    double resp_ema_[N_PRODUCTS]{};  // respsell: trailing mean of hour-start prices (0: not started)
};

double now_ms();

// Defaults with experiment overrides from DC11_OPTIONS ("wage=2 hold=0.95 rival=0.5 timing=1 final=1 stress=2
// grid=2 site=1 radius=4 rounds=6 first=1 firstfund=0 budget=5000000").
Options options_from_env();
Options parse_options(const char* text);  // "key=value ..." (nullptr: defaults)
}
