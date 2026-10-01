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

namespace bcsell { struct Model; struct Tracker; }

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
    bool afford_animal = false;  // affordanimal=1 (dc12): a short animal purchase moves to its first affordable hour
    // buyahead=1 (dc12): an animal is bought at the first hour the cash covers it and every purchase planned before it, not in the
    // hour before its pickup (M&M buys the day-3 cow at h6-9 and places it at h14-16; an unplaced animal waits in the shed).
    bool buy_ahead = false;
    // saleslots=1 (dc12; Imitation's audit): the first hour's 10 engine order slots are shared by hires (one order each) and the
    // seller's dawn sales. dc11 ordered up to 10 hires at the first hour, so on 9-10-hire dawns the seller's h0 lots were dropped
    // (strawberry decided 1.80 / executed 0.18 per dawn) and went out at h1 behind the opponent's h0 lot. With it the first wave
    // leaves one slot per product with dawn stock; the rest of the hires are ordered an hour later (start at h2).
    // saleslots=3: the same dawn decision, but a sale takes a hire's first-hour slot only when routing again with the smaller
    // first wave keeps the crew and drops no required stop (on 13-hire days a later start drops waterings / plantings).
    // saleslots=2: one slot per thin product the seller's dawn decision (choose_sales at h0 on the dawn stock, no receipts) sells,
    // plus the wheatcash dawn sale; the top teams keep 1-2 h0 slots and sell on crowded days only.
    int sale_slots = 0;
    // cashroute=lambda (dc12): when the plain plan is short of cash at hour t, route again with every unit deposited by t earning
    // lambda x its price on top (the cash constraint's shadow price in the deposit values), purchases kept at their hours; before
    // the deferral variants. M&M deposits and sells each morning's collections by midday and buys at h5-h9.
    double cash_route = 0;
    // downprobe=R (dc12, speed): the hire search's step down (dropany) tries each dissolution candidate with R search rounds and
    // runs the full search once, on the first candidate that fits (else the least late); the step down was 71% of routing time.
    int down_probe = 0;
    // depcredit=lambda dephour=H (dc12, for Imitation's both-halves test with mmpolicy): deposits of thin products (not wheat /
    // fertilizer) by hour H earn lambda x the dawn price on top of the DP's deposit value. Our workers collect the morning's milk /
    // eggs / wool as early as M&M but carry them to the evening (milk deposited by h2: 2% vs M&M 21%), because the evening-selling DP
    // gives early deposits no value; early deposits pay only with early selling, so this key is judged with an early seller.
    double dep_credit = 0;
    int dep_hour = 8;
    bool dep_fert = false;
    // cashseeds=0 (dc12): with cashsell, a short seed order fails the funding check again (default 1: tolerated, the executor buys
    // what cash covers and skips the unseeded plantings). The tolerance lets animal purchases crowd out seeds silently (wide bed:
    // cashsell funds more geese, 7 fewer strawberries); without it the ladder trims by its order instead.
    bool cash_seeds = true;
    // skipwater=1 (dc12): when the executor skips a planting (seeds short under cashsell), that tile's later waters / fertilizing
    // today are skipped too. Before, the WATER on the empty tile failed, and the funding check rejected the whole plan for it: the
    // deferred-cow variants of days 2-4 (M&M's intent: cow bought at h6-10 from the morning's sales) failed this way on 83-88% of
    // fallback days (Weaknesses' D3 triage; DC11_FUNDDEBUG: op 9 on an empty tile).
    bool skip_water = false;
    // achieve=1 (dc12, implies skipwater): the funding ladder no longer returns the first stress-funded variant. It evaluates the plain
    // attempt and every deferral variant and keeps the one that achieves most: new animals kept (value), then the fewest plantings
    // skipped for seed cash (seed value), then the fewest dropped stops. First-funded took the plain plan with half its plantings
    // skipped (skipwater alone) or rejected every plan that buys M&M's h6-10 cow and runs one seed short (without it).
    bool achieve = false;
    int achieve_mode = 1;  // achieve=2: the search runs only when the first funded plan skipped plantings for seed cash (or none was funded); 3: as 2, variants compared by value (a skipped seed dollar = 4 animal dollars)
    // seedlag=1 (dc12, with achieve): the deferral ladder also tries variants whose short purchases are funded animals first, the
    // animals released at the variant's hour and the seeds 4 / 8 h later (M&M's day 3: cow at h6-10 from the morning's sales, the
    // seeds after). One release hour for all purchases planted the seeds before the cash came (F6) or placed the cow too late (F14).
    bool seed_lag = false;
    // feedcost=w (dc12): the router charges w x the wheat price per feed-wheat unit picked up when the shed's wheat is bought
    // (Problem::wheat_buy_cost), so routes feed from the day's harvested wheat when they can (M&M's day 3).
    double feed_cost = 0;
    // sellmodel=1 (dc12, I1): strawberry / egg / milk / wool are sold as BC's learned M&M seller samples (<model>.sellmodel, loaded
    // by the agent; dc11_local/sellmodel.hpp) instead of the DP's lots; tonight's night-room sales and this hour's purchase cover
    // stay on top, and the last day stays the DP's (liquidation).
    bool sell_model_on = false;
    int sell_model_mode = 1;    // sellmodel=1: sample the model's units; 2: BC's threshold decode (deterministic)
    int sell_model_last = 28;   // sellmodellast=D: the model sells on days <= D (29: through the last day), the DP after
    int sell_model_first = 0;   // sellmodelfirst=D: ... and on days >= D, the DP before
    const bcsell::Model* sell_model = nullptr;
    bcsell::Tracker* sell_tracker = nullptr;  // level models (v5): the game's trailing market levels, updated every hour
    unsigned long long sell_seed = 0;  // per-game constant for the model's sampling (set by the agent)
    // keepfed=1 (dc12): an animal unfed yesterday is fed whenever its remaining production (units until day 29 x price) is worth
    // more than feeding it every other day to the end. Full games vs our clones: ~1.8 escapes / game before day 25 (the rival 0.2-1.2);
    // the traced escapes were intent groups of unfed sheep with feed 0 (network), e.g. day 18 of a 24-animal farm.
    bool keep_fed = false;
    // nighttrim=1 (dc12): when tonight's cargo still exceeds the shed room after the router's deposit repairs, remove pure output
    // stops (harvest / collect only) until it fits; their product stays on the animal / crop. Full games vs our clones: ~10-12 units
    // per game destroyed at the night deposit (days 21-29; workers' pockets 138-148 units on end-game nights).
    bool night_trim = false;
    // affordall=1 (dc12): when the plain plan is unfunded, first try a variant where each purchase gets its own earliest affordable
    // hour on the day's cash timeline (dawn money + the shed stock's sale value as the plain plan deposits it), animals before seeds.
    bool afford_all = false;
    // bestvariant=1 (dc12): the funding ladder evaluates the deferral variants and keeps the funded one with the fewest dropped
    // required stops (earliest on ties) instead of the first funded one.
    bool best_variant = false;
    // holdown=1 (dc12): the seller's hold value (units kept overnight, valued at 0.95 x the book tomorrow noon) counts our own supply
    // that reaches the market by then: tonight's pocket carry + half a day's production (today's as the estimate). The linear market
    // makes the held units the marginal ones: sale_value(inventory + own supply, held). Audit M5: the hold value ignored our own
    // next-day output; Imitation: half of M&M's price lead is which days it sells on.
    bool hold_own = false;
    // survivalfloor=1 (dc12): when every level is unfunded, the SurvivalOnly level's routed plan is executed anyway instead of an idle
    // day (idle: the animals go unfed and escape after two days; seat-swap game 114911253: $10 at dawn day 9 -> 13 escapes, -186k).
    bool survival_floor = false;  // depfert=1: fertilizer deposits earn the credit too (the early days' cash; M&M sells it by h8 on day 3)
    // reserve=0 (Sep 29, v95): no next-dawn reserve in the funding check (end assets >= animals x wheat price x 1.2 + 30). With
    // cashsell the dawn sale finances tomorrow's feed; the reserve made the day-6 wave trim a cow and end with cash unspent.
    bool dawn_reserve = true;
    // reservek=X (dc12): the fixed slack in the next-dawn reserve (tomorrow's feed wheat x 1.2 + X). Day 3 on M&M's intent: the variants
    // that sell the dawn wheat (M&M's cash) end $2-8 short of it; reserve=0 (no reserve at all) costs ~5k in games mid-game (fire-sales
    // for tomorrow's feed, Imitation's ablation).
    double reserve_slack = 30;
    // reservenet=1 (dc12): the next-dawn reserve is tomorrow's feed wheat net of our own wheat: tonight's shed and pockets plus the
    // wheat harvestable tomorrow (M&M feeds from its harvest); no fixed slack. The one-day funding horizon keeps tomorrow's feed.
    bool reserve_net = false;
    // nofire=1 (dc12): a purchase the day's sales cannot cover by its hour gets no fire-sale. The cashsell bonus reverts to none (was
    // kept at its maximum, +400% on revenue up to the purchase: the seller dumped stock for a purchase that still failed), and the
    // executor's cover step sells only when that covers the hour's purchases. Imitation: every weaker next-dawn reserve won G1 d2-5
    // and lost in games, the sub taking the dumped room.
    bool no_fire = false;
    // latehire=1 (dc12): a hire the funding simulation cannot pay is ordered later in the day (Problem::cash_wave / cash_hire_hour)
    // instead of shrinking the crew; M&M's day 1 hires its third worker at h2 from the first fertilizer sale (we hired 1 vs 3).
    bool late_hire = false;
    // splitfert=D (dc12): on days 3..D-1 milk / wool / egg harvest stops carry no fertilizer collection (its own optional stop)
    int split_fert = 0;
    // nearanimals=1 / 2 / 3 (dc12, from the Codex early-products work): days 0-5 new sheep / cows at the free sites nearest the shed;
    // 2: sheep placed before cows; 3: every day, geese too; 4: cows and sheep alternate in placement order (balanced);
    // 5: one cow first, then the sheep, then the other cows (M&M's census: cow at ring 0, sheep at ring 1, cows at ring 2);
    // 6: as 5 on days 0-9, and cows / sheep placed before geese (M&M builds pastures and places pasture animals first). 0: off.
    int near_animals = 0;
    // cashshadow=1 (dc12, with achieve): variants with a deposit bonus (lambda 0.5 / 1 x price) before the cash-bound hour join the
    // achieve search (compile_level), so routes that bring cash early compete with the deferral variants.
    bool cash_shadow = false;
    // hirecheck=1 (dc12 bug fix, m14): the funding check fails a plan when fewer hires are filled than ordered in an hour. Before, each
    // planned hire compared only the first hire order's fill, so a partial hire wave (dawn cash for 6 of 9) passed as funded, and the
    // unhired workers' routes (a land day's plantings) silently vanished in the simulation and live (quick-60 day 6: 2 of 16 planted).
    bool hire_check = false;
    // animalfirst=1 (dc12): the deferral variants fund animals before seeds (the DC12_ANIMALFIRST probe as a per-agent key). Own games,
    // day 9: a 4-sheep ask funded after the day's seeds came at h10 and 3 of 4 pastures were dropped; M&M buys animals in the morning.
    bool animal_first = false;
    int animal_first_mode = 0;  // animalfirst=2 (with achieve): both funding orders compete in the variant search
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
    // landfirst=1 (dc12): when the day's full ask is unfunded and it includes land, the land alone (other new entities dropped) is
    // tried before the NoLand level (compile_day_once).
    bool land_first = false;
    // landpull=N (dc12): after a land day is funded with the land moved to its first affordable hour, that hour is recomputed on
    // the funded plan (up to N times) and the land is pulled earlier when the re-routed plan stays funded. The first hour came from
    // the plain plan, routed with the land at h0 (its crew plants the new tiles instead of depositing sellable stock), so it was
    // late (M&M states: our land 1.5-3 h after M&M's with the same dawn cash).
    int land_pull = 0;
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
    double skipped_seed = 0;           // achieve: seed value of the plantings the funding simulation's executor skipped (seeds short)
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
    int consumed(int hour, int animal) const { return consumed_[hour][animal]; }  // buyahead: planned animals bought earlier
private:
    Plan plan_;
    Options options_;
    int day_ = -1;
    DayMarket market_;
    double band_acc_[N_PRODUCTS]{};  // bandsell: fractional units carried to the next hour
    double resp_ema_[N_PRODUCTS]{};  // respsell: trailing mean of hour-start prices (0: not started)
    int ahead_[N_ANIMALS]{};            // buyahead: animals bought before their planned hour, not yet matched
    int consumed_[HOURS][N_ANIMALS]{};  // buyahead: planned animal units at each hour covered by an earlier purchase
    bool unplanted_[BOARD * BOARD]{};   // skipwater: tiles whose planting was skipped today (seeds short)
    double skipped_seed_ = 0;           // achieve: seed value of today's skipped plantings
    int sold_today_[N_PRODUCTS]{};      // sellmodel: our units ordered for sale since hour 0 (a model input)
public:
    double skipped_seed() const { return skipped_seed_; }
};

double now_ms();

// Defaults with experiment overrides from DC11_OPTIONS ("wage=2 hold=0.95 rival=0.5 timing=1 final=1 stress=2
// grid=2 site=1 radius=4 rounds=6 first=1 firstfund=0 budget=5000000").
Options options_from_env();
Options parse_options(const char* text);  // "key=value ..." (nullptr: defaults)
}
