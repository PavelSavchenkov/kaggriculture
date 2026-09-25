# Network vs day compiler: conceptual gaps and which decisions to move (Sep 25)

Question: are there conceptual gaps in the task formulation (network DayIntent + deterministic
day compiler) that justify moving decisions to the network? Which ones?

Sources: `designs/day_intent.md`, `designs/day_compiler.md`, the Sep 21 full design
(`work/opus55_analysis_sep24/design.txt`), `ideas.txt`, the Sep 24 boundary analysis
(`work/opus55_analysis_sep24/REPORT.md` section 7), and new games of the Vadim-opening candidate:
Local-LB panel, 160 paired games per probe unless noted (5 behaviours, seeds 700-715, both seats).
Probes are env-gated patches in `exp_patch/` that measure the value of a concept; they are not
proposed implementations.

## 0. Answer

Rule we apply (ideas.txt, Sep 21 design "final principle", user preference): the network owns
decisions whose value is realised on later days; the compiler owns within-day mechanics; move a
decision only when the compiler fails at it consistently for a conceptual reason.

1. **Root gap: no continuation value anywhere.** Every cross-day trade-off is either imitated by
   the network (BC probabilities, no value) or valued by a fixed one-day rule in the compiler
   (hold factor 0.95, next-dawn cash reserve, 1 wheat per animal, "most expensive first" trims,
   $2 per returned unit). The Sep 21 design asked for calibrated next-state values in five compiler
   places and a value-learning stage; none exists. Most weak points found today trace to this.
2. **Move one decision now: what to give up when the day's plan is over budget.** The DayIntent is
   exact and budget-blind, the compiler's trims are preference-blind, and the design already said
   "request a revised plan from the policy". Evidence: 91% of trimmed units fall on the investment
   days 0, 6, 7, 9; letting the network's own preferences pick what to drop gives **+$5.3k
   [+1.4k, +9.2k]** (X10), as good as the best hand rule (+$5.1k, X1) but without a day-specific rule.
   Ordering by probability *per dollar* fails (-$1.4k, X6): BC probability is not value.
3. **Do not move sell/hold (overnight stock) yet**, although it is cross-day and top teams hold much
   more. Both probes collapsed for a mechanical reason: our compiler's horizon ends at night and it
   needs an almost empty shed by morning. Top teams run an inventory cycle across the night (shed
   65-90 units at dawn, more night cargo, 27 units sold at hours 0-5 vs our 11). First gap to close
   is the compiler's day boundary; then a network stock target or a learned terminal value.
4. **Keep in the compiler** (no conceptual failure, or the failure is within-day): workforce,
   routes and returns, tile placement, within-day sale timing, and the opponent forecast (a
   forecasting gap fixable from visible biology; optionally a learned sale-timing head). No need for
   input-stock targets: the network is not hurt by our execution-driven stock differences (X9).
5. **Add a value model** (learned V of the dawn state, margin or win) as the missing piece for both
   sides: the decoder ranks budget-feasible candidate intents with it, and the compiler takes its
   terminal values (per-product value of held stock, value of cash) instead of fixed constants.

## 1. Evidence per candidate decision

| Decision | Owner now | Cross-day? | Consistent failure? | Probe | Verdict |
|---|---|---|---|---|---|
| What to drop when over budget | compiler rule (most expensive first) | yes: a day-0 sheep pays ~$2.8k over 25 days, a melon ~$0.9k | yes: day-0 sheep dropped in 100% of games; 91% of trims on days 0/6/7/9 | X1 hand rule +5.1k; X6 network per-$ order -1.4k; **X10 network probability order +5.3k** | **move to network** (decoder re-plans under a compiler budget) |
| Cash kept overnight (reserve) | compiler rule | partly | yes on day 0 (ignores overnight fertilizer) | part of X1/X10 | keep in compiler, compute from certain income |
| Overnight product stock (sell vs hold) | compiler, DP horizon ends tonight, hold = 0.95 x tomorrow | yes | all-or-nothing: strawberry/egg/wool sold out on 38-46% of days (top 2-15%) | X8 teacher fraction cap -23.6k; X11 teacher stock -13.2k (shed overflow) | **not yet**: first let the compiler plan across the night |
| Input stock (seeds, wheat, fertilizer) | compiler, just in time | yes | large state differences (seeds 0 vs 1-7, fertilizer 0-1 vs 4-12) | X9 network sees teacher stocks: +0.3k [-2.6k, +3.5k] | keep in compiler (no drift harm) |
| Workforce (hires) | compiler | little | yes, over-hiring: returns add 1.4 hires/day | X5 (drop returns) -1.8k | keep in compiler; fix route construction |
| Tile placement | compiler | yes (animals permanent) | no: geometry at top-team level | - | keep; low joint-optimization value |
| Opponent supply forecast | compiler, trailing 3-day mean | forecast | yes: error 0.47-0.85 of volume, 1.4-1.6 for melons | X2/X4 (race probe) +2.4k / +6.8k | compiler fix from visible biology; optional learned head |
| Within-day sale timing, returns | compiler | no | race losses (melons) | X2 | keep in compiler |

## 2. Details

### 2.1 Budget allocation (move)

- The network asks for top-team investments in states where they are unaffordable: day 6 asks for
  $3,242 of animals and land (plus ~15 crops) with a median $1,134 cash; day 0 is ~$80 over budget.
  Trims: day 0 100% of games, day 6 97%, day 7 70%, day 9 38%.
- The compiler's rule drops the most expensive entity (the day-0 sheep, then day-6 cows). Crops
  first on day 0 fixes day 0 (+5.1k, X1), but crops first on all days was -3.4k (earlier panel on
  v11): a fixed rule is right on one day and wrong on another.
- The network's preference is the right signal when used as "least likely to be chosen": on day 0
  it drops one wheat and one melon and keeps the third sheep (day-1 cash $12, top teams $29). X10:
  +$5,290 [+1,404, +9,244], wins 94% → 100% (+6.2 pts), positive against all 5 opponents.
- Dividing by cost (the current `DC10_TRIM_MODEL` in the other session's code) inverts this: an
  expensive unit has a small loss per dollar, so the sheep goes first (X6: -$1.4k, identical to the
  old rule on day 0).
- Proper form: the compiler returns the funding shortfall (cash plus certain income minus wages and
  feed); the decoder picks the most probable intent that fits it (a min-loss covering knapsack over
  units, solved with the existing count DP), and the compiler compiles that. With a value model,
  rank the feasible candidates by value instead of probability.

### 2.2 Overnight stock (sell vs hold): the day boundary is the gap

- The sale DP optimises today's hours; units left tonight get a fixed terminal value
  (0.95 x price after 12 hours of demand, trailing opponent flow, no own next-day output). With a
  steep price curve this gives corner solutions.
- Top teams hold stock continuously (strawberry 15 at dawn vs our 8, egg 7 vs 3.6, fertilizer 8.6
  vs 2.6), keep the shed at 65-90 units from day 12 (ours 10-54 until day 20), carry more overnight
  (66 vs 57 units per night) and sell 27 units per day at hours 0-5 (ours 11).
- Our compiler plans tonight's room as "shed sold down by tonight": it returns goods by hour 20
  and sells them in the evening (1,038 units at hours 18-23 per game vs 577), which is also what
  drives the extra return hires.
- Probes: a teacher sell fraction (X8) and a teacher overnight stock (X11) both overflow the shed
  (100 units at dawn, 15-50 units destroyed per night) and lose 13-24k. The holding itself is not
  shown to be bad; our one-day capacity model cannot carry stock.
- Fix order: (1) compiler capacity and sales across the night (tonight's deposit plus tomorrow
  morning's sales as one plan, dawn sell-down as an option); (2) then give holding a cross-day value:
  a network overnight-stock target per product (exact replay labels) or a learned terminal value per
  product. Test (2) only after (1).

### 2.3 Workforce, routes and placement (compiler)

- The work the network asks for fits 10.25 hires (mode 11, like top teams); the return step adds
  1.4 hires/day. No reason to give the network a workforce target (the Sep 24 proposal targeted an
  older compiler that under-hired).
- Placement is at top-team level: animals 1.3-1.45 tiles from the shed (top 1.74, with a smaller
  herd), crops 4.6-4.7 (same); same-day cohorts as compact as top teams' or more; the day's worked
  tiles are as compact (spanning tree 1.02 vs 1.00 per tile).
- Routes are where walking is lost: 2.1 moves per unit of the day's spanning tree vs 1.76; per
  worker-day 11.0 actions and 9.6 moves vs 12.0 and 9.2. Top teams combine animal and crop work in
  one trip 64% of the time (ours 50%) and cover more, better-clustered tiles per trip (4.9 tiles,
  tour 1.28 per tile vs 4.6 and 1.45). The joint "placement + routes" gain is mostly in route
  construction (chain shed → animals → crops, as top teams do); placement changes would add little.

### 2.4 Opponent forecast (compiler, optional learned head)

- Opponent daily sales are production-driven: the trailing 3-day mean misses 47-85% of volume and
  140-160% for melons; "sales follow today's harvest" cuts melon error to 0.44-0.50, wool to
  0.47-0.56, milk to 0.43-0.50.
- Harvest timing follows public biology (animal and crop ages on the opponent's farm), so the
  compiler can forecast it legally. Sale timing is policy-dependent; a small learned head trained
  on replays (labels: opponent sales by hour) is the natural network contribution. It is a
  forecast, not a decision.

### 2.5 What the probes say about BC in general

- BC encodes what teachers do, not what things are worth. Where our states leave the teacher
  distribution (over budget, different stock), BC can still express preferences (X10 works) but
  cannot trade value against cost (X6 fails). Value-dependent choices need search or a value model:
  the handoff's per-dawn search kept +9.4k with a wrong opponent model.
- Execution-driven input differences did not hurt this network (X9), unlike agent_sep23.

## 3. Proposed formulation changes (in order)

Details: `OPPORTUNITY_night_inventory.md` (item 2) and `VALUE_MODEL.md` (item 3).

1. Budget revision loop: compiler reports the shortfall; decoder re-decodes the most probable
   budget-feasible intent (min-loss covering knapsack). Replaces hand trims on all days. Measured
   value of the probe version: +5.3k.
2. Compiler horizon across the night: plan tonight's deposit together with tomorrow morning's
   sales; allow a dawn sell-down. Prerequisite for any holding decision and a likely wage saving.
3. Value model V(dawn state) from the 1.45M replay days plus our games: rank K candidate intents;
   give the compiler per-product terminal values and a cash value.
4. Then test a network overnight-stock target per product (exact labels) against the value-model
   terminal values.
5. Opponent forecast from visible biology, plus an optional learned sale-timing head.

Not proposed: network targets for workforce, input stocks or placement.

## 4. Probe list

| Probe | Flags (exp_patch) | Result vs baseline [95% CI] |
|---|---|---|
| X1 day-0 crops-first trim + no day-0 reserve | `OPUS_TRIM_D0_CROPS=1 DC10_RESERVE_FROM_DAY=1` | +5,087 [+1,759, +8,611] |
| X6 network trim order per dollar | `DC10_TRIM_MODEL=1` | -1,426 [-6,324, +2,288] |
| X7 X6 + no day-0 reserve | `+ DC10_RESERVE_FROM_DAY=1` | -1,407 [-6,234, +2,425] |
| X10 network trim order by probability | `DC10_TRIM_MODEL=1 OPUS_TRIM_LOSS_ONLY=1 DC10_RESERVE_FROM_DAY=1` | +5,290 [+1,404, +9,244]; wins 94% → 100% |
| X8 teacher daily sell fraction | `OPUS_SELL_CAP=1` | -23,581 [-34,789, -15,918] (128 games, stopped) |
| X9 network sees teacher-typical stocks | `OPUS_CANON=1` | +258 [-2,566, +3,466] |
| X11 teacher overnight stock (strawberry, egg, fertilizer) | `OPUS_HOLD=1` | -13,180 [-19,881, -8,387] (47 games, stopped) |

Tools: `tools/layout_audit.cpp` (day spanning tree), `tools/trip_audit.cpp` (worker trips),
`tools/cohort_audit.cpp` (same-day cohorts), ledger v2 (`tools/ledger2`: seeds, shed distances),
`q20`-`q28.py`.
