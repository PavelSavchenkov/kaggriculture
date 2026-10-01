# 01 Progression: how the agent evolved, and why

This is the story in order. Each phase lists what we tried, why, what happened, and what we carried forward. Numbers are paired
margins in dollars per game (our money minus the opponent's) unless stated otherwise. "SE" is the standard error of the paired mean.
Kaggle scores are skill ratings in a pool that kept getting stronger, so they are comparable only within a few hours of each other.

## Kaggle score timeline (team submissions, last 50)

| Date | Line | Best public score of the period |
|---|---|---|
| Sep 17 | 5-day suffix search (C++ search, 27 s per game) | 1900.6 |
| Sep 18–24 | RL / PPO from scratch, raw-action BC on top players, KNN rules, replay clones | 667 – 1950 |
| Sep 25 | Public notebooks (reference) | 2085 – 2333 |
| Sep 25 | **BC Opus v12 + day compiler** (`pavel-bc-opus-v12-robust`) | **2740.7** |
| Sep 26 | Wider decode, dc11 compiler, learned opponent forecaster | 2815 – 2839 |
| Sep 27 | Packages (rival, tieall, timing 0.5, Q4, ensembles D / E) | 2828 – 2867 |
| Sep 28 | Fresh-data main network v17, econ keys, day-3 crop push (`d3crop`) | **2878.0** (best ever) |
| Sep 29 | M&M hybrid, melon push, dc12 funding keys (`m3`, the base) | 2792 – 2824 (m3 settled 2807.8) |
| Sep 30 | CMA decode (2709), fc2 forecaster (2727), final pair: base re-submit + honest1 | pool much harder |

The table in [kaggle_submissions.csv](kaggle_submissions.csv) has every submission with its description.

---

## Phase 0 — Engine and fixed courses (Aug 26 – Aug 31)

**What.** A fast C++ game engine (`fast_game_engine/`), the written rules (`prompts/game_rules.md`), and the first agents as
fixed action tapes. A strong course was compiled offline in C++ and exported into a `main.py` table (`submissions/aug28-*`,
`aug29-fixed-weed-*`). Then came shop-dependent variants (`aug30-one-random-shop`, `two-random-shops`, `aug31-four-random-shop`),
selected in frozen leagues of up to 262,144 games.

**Why.** Kaggle runs `main.py` with a 1 s per-turn limit plus a 60 s overage bank. A tape needs almost no runtime, and a C++
engine lets us test policies by the hundred thousand.

**Learned.** The game is a 30-day market economy for two farms. Both farms sell into the same shops, so prices couple them.
Shops (which products sell) change the best plan, and a course that does not adapt loses whenever the shops differ.

## Phase 1 — Composition search, replay borrowing, day solver (Sep 1 – Sep 9)

**What.** We searched over farm compositions (dated product / count / lifetime plans). Strong courses were borrowed from public
replays (e.g. Justin Lee's), and a day solver (`day_solver/`, V30) built legal worker schedules for one day. Adaptive rules
(cows vs sheep from observed wool demand) were added. Submissions: `sep7-shop-herd-adaptive-v1` (Kaggle 56074695),
`sep8-composition-adaptive-v1` (56101451). Full records: `handoffs/sep07_agent/`, `handoffs/sep08_agent/`.

**Learned (still true at the end):**
- Prices couple both farms. Always measure own money, rival money and margin separately. A branch forecast as +$3.6k margin
  was −$13.7k in exact play, because the rival gained more than we did.
- An oracle ablation showed what matters for planning: knowing future shops and rival trades fixes almost all branch errors
  (regret $3,249 -> $225). This motivated the later opponent forecaster.
- The day solver saves labour inside a fixed plan, but it does not own the economy. A good schedule cannot fix a bad plan.

## Phase 2 — Components and search (Sep 9 – Sep 17)

**What.** A fast day-solver estimator, tile and sales planners, a day policy for up to 11 workers, a replay-backed RL pipeline,
and a 5-day suffix search agent (27 s of search per game) -> Kaggle 1900.6 (`sep17-suffix-5d-28s-v1`).

**Learned.** Hand-built search and solvers plateaued well below the top of the leaderboard (~3000). The strongest
public notebooks (2085–2333) were simple engines with good economic rules.

## Phase 3 — Learning from scratch and raw-action cloning (Sep 18 – Sep 24)

**What.** In parallel, the team tried:
- RL / PPO from scratch: V5 / V6 lines, 820–1730.
- Behaviour cloning of raw actions on the top players Majkel and M&M: 667–1360.
- KNN animal rules: 1546.
- "Replay champion" clones of top submissions: 1447–1950.

**Learned.** Raw-action models must learn routing, legality and economics at once. They were weak and slow to improve. This
pushed us to split the problem: **a network decides WHAT the farm should do each day; deterministic code decides HOW.**

## Phase 4 — DayIntent BC + day compiler (Sep 23 – Sep 25): the big jump

**What.** `agent_sep23` introduced the split.
- Every dawn a behaviour-cloned network predicts one **DayIntent**: crops / animals to add, land, and for each group how many
  to water, feed, harvest, care for and collect.
- A C++ **day compiler** turns the intent into hourly worker actions and market orders (tiles, routes, hires, purchases, sales).

`pipeline_sep25_handoff/` (BC Opus) scaled this up:
- Network `v12_cond`: 2.3M parameters, trained on 1.45M days of top Kaggle players, conditioned on player strength.
- Output design: 101-way count categoricals, decoded exactly by MAP dynamic programming; whole-farm totals as total x type
  shares.
- Many compiler fixes, each found by tracing lost full games.
- Kaggle `pavel-bc-opus-v12-robust` = **2740.7**, about +400 over the best notebook; `wide` = 2815.

**Why it worked.** The network copies what strong players plan (herd size, crops, timing), learned from a lot of data. The
compiler guarantees legal, cheap execution, which a network is bad at.

**Key fixes from traced losses (pipeline_sep25_handoff/README.md):**
- Trim an unfunded day one unit at a time instead of dropping it: 14/64 -> 42/64 wins.
- On day 0, drop crops before animals: +$6.6k; the third sheep had been lost in 100% of games.
- Market order slots. The engine fills both players' orders slot by slot at the same quote, so listing sales by revenue at
  stake (`sell_order 2`) won 39/40 mirror games (+$11.6k).

## Phase 5 — dc11 compiler overhaul and the learned forecaster (Sep 26)

**What.** dc11 (`experiments/v10/sep25_compiler_overhaul`) replaced the compiler.
- Compile time: 2–3 s per game (was 16–114 s).
- Wages: $224 per day (was $355).
- Gates: v19 36-4 (+3.9k) vs the frozen compiler; v29 39-1 (+7.4k).

A learned **opponent-sales forecaster** arrived (`tf_lin`: causal transformer over days, plus an intra-day re-forecast head).
It was the first forecaster to beat our own lineage: exact Local-LB +2.6–2.9k, pinned Kaggle replays +3.35k. Kaggle:
dc11 2829, tf 2839.

**Learned:**
- **Check model assumptions against the engine source before tuning knobs.** v29's "interleave" fix priced same-turn sales the
  way the engine executes them (both quotes before either trade): +1.6k to +4.6k. Knobs tuned under the wrong model flipped sign
  after the fix (`timing`).
- What matters in the forecast is timing, more than totals: the timing oracle was worth +8.3k vs +4.5k for totals.
- Held-out forecaster loss (NLL) did not pick the best forecaster. Pinned game replays did.

## Phase 6 — Packages, ensembles, the plateau (Sep 27)

**What.**
- `big2` forecaster (intra-day head plus stock / visible inputs), `rival=1` (the seller values the rival's loss), `tieall`
  (sell now on price ties), `timing=0.5` (released only after a Kaggle A/B, the user's rule).
- The day-10 4th quadrant (Q4) and ensembles: E = a main network + four older members, averaged.
- Kaggle 2828–2867 (E 2866.8).

**Learned. The Kaggle rating plateau.** Every step scored +1–5k per game on pinned replays, yet ratings stayed at
2828–2849 ([03_EVALUATION.md](03_EVALUATION.md)). Two reasons:
- A live A/B with ~60 games per arm cannot resolve differences under ~3k per game, and +1k per game is only about 20 rating
  points.
- Pinned beds cannot show how opponents react.

The old agent lost to the top 10 mostly on sale price (73% of our sales at h18–23 vs their 34%).

## Phase 7 — Fresh-data networks and the best Kaggle score (Sep 28)

**What.**
- Networks retrained on fresh games: v15 / v16 / v17, then `v17g6ft5`, the main network of every final agent.
- Fresh mains gained +3–4k per game on reactive beds. Pinned beds had hidden this, and the older v12 had been a lucky,
  top-decile seed.
- The `econm6` package added collectmin / pricefloor / a day-6 melon top-up. The forecaster alone was worth 4.2k (removing it:
  −4.21k, 0/20).
- `startcomplete=1` fixed a "starved opening" cliff: a day-0 plan that left cash unspent cost 15–20k.
- The day-3 crop push (`qpush`) gave `pavel-bc-opus-v17d-dc11v59-ens-d3crop` = **2878.0**, our best Kaggle score.

**Learned:**
- Fresh networks win mostly by denial: the opponent earns less (`fresh-net-denial`).
- Judge network changes by margin on reactive beds, single network vs single network.
- A "0/20, −20k, land 3" collapse row was usually the starved-opening cliff, not the change under test.

## Phase 8 — Imitating the #1 team, and the dc12 rework (Sep 29)

**What.** M&M was #1 on Kaggle.
- We trained copy networks on M&M's games and a hybrid: `hyb_nolp`, the M&M main at weight 0.5 plus E's members, 2824.
- The Day compiler session isolated compiler differences with "teacher days": one day replayed from M&M's recorded dawn, with
  M&M's intent, against the recorded opponent. That gave the **dc12** keys:
  - `cashsell / wheatcash / reserve=0`: sell the morning's goods by midday and fund purchases with them, like M&M.
  - `dropany=6`: the hire search may dissolve the least-loaded route.
  - `saleslots=3`: dawn sale orders get engine order slots.
  - `nighttrim`, and `survivalfloor` (a cash floor; without it one unfunded day lost 186k).
- Package **m3** = d3crop + melon push days 6–8 (`earlycrop 4 6 8 2`) + dc12 keys -> Kaggle 56690263, settled 2807.8.
  This is the "base".
- A teammate's CMA-ES search over decode values reached Local-LB #1 but was worse live (2792; 2709 on m3). From then on
  the user banned CMA and RL / PPO agents in any role.

**Learned:**
- Against a fixed, replayed opponent our agent earns what M&M earns. The whole gap is the opponent's extra income against us
  (`mm-gap-is-denial`): a thin price lead on strawberry, milk and wool (~$3–4.5k per game). Our collections come in pulses and
  our seller sells them straight through.
- Compare live agents only in the same time window. A first read of hyb_nolp looked negative purely because the pool
  changed during the day.

## Phase 9 — Copy M&M for 48 hours; the forecaster and the opening (Sep 30, day)

**What.** All sessions worked on closing the M&M gap, with gated beds (G1 / G2 / G3 = M&M's own worlds; swap = real top-30
worlds).
- **fc2 forecaster**: the old forecaster's training data was fixed (stale data, a wrong hour-rotation augmentation, synthetic
  lineage games). G3 +1.41k. With the `hirecheck=1 latehire=1` hire-funding bug fix -> Kaggle 56714867.
- **Opening keys.** `splitfert=10`: on days 3–9 a harvest no longer carries the fertilizer collection, so wool and milk reach
  the shed early. `nearanimals=6`: animals are placed near the shed in M&M's order. Together they move our wool to M&M's
  hours: G3 +1.2k, Local-LB +90 rating -> LB #1 `m19-fc2`.
- **The validation bias.** Every local opponent was our own lineage, which sells wool ~3 hours later than real teams.
  So the local beds overstated early-selling gains.
- **The hybrid bed** (realistic opponents: a real team's recorded farm with a learned M&M seller): the opening keys were level
  there (−0.04k, own −1.8k).
- **The live gap is reaction.** Top-30 teams earn +4.6k more against us than against M&M: wheat volume and morning wool / milk.

Closed: dawn seller, evening hold, opening script, wheat floor, regime carry, more compiler time, recency conditioning
([04_LEARNINGS.md](04_LEARNINGS.md)).

## Phase 10 — Local-LB push, the overfit audit, final picks (Sep 30, night)

**Local-LB push.** Weaknesses built an **exact Local-LB judge**: the LB's own code. It reproduces the LB's official games to the
dollar (83 / 83).
- `f1` = #1 with two more fc2 seeds: +922 per game (1.9 SE, seed-clustered), 26 decided games gained / 8 lost. Merged as
  Local-LB #1 (1651).
- Recency conditioning and a saleslots=4 key failed against #1.
- Lineage-weighted forecasters (6x weight on our own games) led for a while. They were dropped on the user's rule: **no part
  tuned to our own lineage**.

**Overfit audit** of the LB top 3, the PR agent and m3:
- `nearanimals=6` is lineage-only: −106 per game (1 day) and −66 (5 days) on live-game continuations against real opponents.
- fc2's training data is 47% our own games in the recent window. It beats the clean forecaster (fc3nvens, no lineage data) by
  1.55k on the LB, but the two are equal on real opponents.
- The CMA decode was confirmed overfit by live play.

**The honest line** keeps only parts supported on real-opponent data (`evidence/HONEST_LINE.md`). Main-network seed soups
lost to the current seed (−307 / −213), so the main network stayed.

**Final Kaggle picks** (user-asked, both fully checked, both validation episodes reproduced exactly; [06_FINAL_AGENTS.md](06_FINAL_AGENTS.md)):
1. The base m3, re-submitted as 56720080: the most-tested agent.
2. **honest1**, 56720831 = base + clean forecaster + splitfert + hire-funding fix (on the m19 build).

honest1 beat the alternative "base + clean forecaster" on 81 fresh live games never used anywhere (+224, SE 70) and on the
recorded-opponent and real top-30 beds.

---

## Why we ended where we did (the main lessons)

1. **Split decisions from execution.** Learned network for WHAT, deterministic compiler for HOW. This was the single biggest jump.
2. **Find bugs by tracing lost games and reading the engine source**, not by tuning knobs. The largest compiler gains came from
   correcting a wrong model of the engine (order slots, same-turn pricing, funding).
3. **The market couples the farms.** Gains come as much from denial (the opponent earns less) as from our own income. Judge by
   margin, and look at own and opponent money separately.
4. **Your test bed's opponent decides what you learn.** Our lineage sells late and replays cannot react. Changes tuned there
   (CMA, nearanimals, lineage-weighted forecasters) won locally and not live. The realistic beds (live-game continuations, hybrid
   opponent, real top-30 worlds) were the ones to trust ([03_EVALUATION.md](03_EVALUATION.md)).
5. **Live ratings are noisy and drift.** Only large behaviour changes move them. Compare within the same window.

## What we would do next

- **Network side: late planting.** #1 loses to its closest siblings on days 18–29 because it plants about 6 fewer strawberry /
  tomato tiles on days 10–13. Strawberries planted on days 6–9 die before the end game. Teach the plan the crop lifetime against
  game end, and validate off the LB (`evidence/team/findings/day_compiler.md`).
- **The mid-game seller against reacting teams.** The live gap is days 10–28: morning wool / milk and wheat volume. Every fixed
  rule we tried lost on our farm, so this needs a seller trained or judged against reacting opponents.
- **Re-select the base's lineage-picked parts** (opening template, herd reach, compiler switches) by leave-one-out on the
  realistic beds.
