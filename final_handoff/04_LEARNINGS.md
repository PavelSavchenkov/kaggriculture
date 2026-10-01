# 04 Learnings: what worked, what failed

Paired margins per game unless stated. "Live" = Kaggle. Each item names the bed it was measured on, because beds disagree
([03_EVALUATION.md](03_EVALUATION.md)). Deeper notes: `evidence/` and the earlier handoffs.

## What worked

### Architecture and network
- **Split WHAT (network DayIntent) from HOW (C++ compiler).** Kaggle 1950 (best raw-action / clone agents) -> 2740 -> 2878.
- **Count outputs as 101-way categoricals, decoded by exact MAP DP**, and farm totals as total x type shares. v1 of this beat
  `agent_sep23` decisively (pipeline_sep25_handoff).
- **Strength conditioning** of the network on per-submission player strength: copy strong players, not the average.
- **Fresh-data networks** (v15 / v16 / v17 on games to Sep 28): +3–4k reactive vs v12. v12 had been a lucky seed.
- **Ensemble of a fresh main + four older members** (members on days 6+): the members alone are worth ~1.8k on the pinned bed.
  Replacing old members with fresh ones lost (−2.1k exact LB).
- **Day-specific decode pushes** with real-opponent support: the day-10 4th quadrant `v219` (−2.4k without it), the day-3 crop
  push `qpush` (best Kaggle score), the melon push on days 6–8 (`earlycrop 4 6 8 2`).

### Compiler / execution
- **Trace lost games; fix the model of the engine.**
  - Unfunded-day trims one unit at a time: 14/64 -> 42/64.
  - Day-0 crops before animals: +6.6k.
  - Same-turn order semantics ("interleave"): +1.6–4.6k.
  - Market order-slot priority `sell_order 2`: 39/40 mirrors.
  - 10 engine orders per turn incl. hires: `saleslots`.
- **dc11 overhaul**: compile 2–3 s / game (was 16–114 s); wages $224 / day (was $355); gates 36-4 to 39-1.
- **Feasibility-first hire search**: decision-identical, −33% compile time. dc12 cut to live keys: identical play, −31% lines.
- **startcomplete** (the starved-opening cliff: unspent day-0 cash cost 15–20k) and **survivalfloor** (one unfunded day:
  −186k -> −7.9k).
- **dc12 funding keys** (`cashsell / wheatcash / reserve=0`), found by teacher-day isolation on M&M's states: pinned +1.5k.
  `dropany=6`: pinned +533.
- **hire-funding fix** (`hirecheck / latehire`): +105 on live continuations, own money.
- **splitfert=10** with a non-fc2 forecaster: +320 (m3's forecaster), +104 (fc3nvens). Products reach the shed before the
  fertilizer detour.

### Forecaster and seller
- **A learned opponent-sales forecaster** (causal transformer over days + intra-day re-forecast head) was the largest single
  component: removing it cost −4.21k (0/20). First version +2.6–2.9k on the exact LB, +3.35k on pinned Kaggle games.
- **Fix the forecaster's training data**, not its outputs (fc2):
  - remove stale data, a wrong hour-rotation augmentation and synthetic lineage games;
  - add yesterday's hourly sales of all products.
  - Result: G3 +1.41k.
- **Seed ensembles** (average 3 seeds of one recipe): free variance reduction. The runtime averages any number of
  `.forecast_tf.N`.
- **Clean forecaster fc3nvens** (no games of our own agents, no bed worlds): equal to fc2ens on real opponents (+11, SE 226),
  and on the base +611 (SE 237) on live-game continuations.
- `tieall=1` (sell now on ties): +1.6k on top-30 pinned games. `rival=1` pays only together with the learned forecaster.

### Evaluation
- **The exact Local-LB judge** (the LB's own code) predicts official LB games to the dollar.
- **Live-game continuations** with exact identity: the cleanest real-opponent read we had. It replicated on a fresh set
  (81 games) in the final decision.
- **The hybrid opponent bed** (real farm + learned M&M seller): the first local bed with realistic sale hours. It reproduced the
  live gap pattern.

## What failed or was closed (do not re-open without new evidence)

### Network / decode
- Raw-action BC and RL / PPO from scratch: 667–1730 live.
- A teammate's RL / PPO on the day-plan layer: live 2805–2833, later 2715. The user rules all RL / PPO agents overfit.
- CMA-ES decode values: Local-LB #1, live 2709 vs 2808. Banned in any role.
- Any fine-tune of the v12 main as the main network: −1–2k. Use goal adapters or ensemble members instead.
- M&M copy as the main network: −2.1k vs d3crop on the league. Hybrid (copy at weight 0.5): live not better vs the top 10 (4/16).
- M&M "dials" (macro conditioning): median plans −3.7k.
- Recency conditioning 1.10 / 1.20 / members: +1.3k on the hybrid bed but 40% / 10% / 37% vs #1 on the exact LB.
- Main-network seed soups (averaging 4 or 9 seeds): −307 / −213 on live continuations, −964 / −388 hybrid. The lottery seed
  `v17g6ft5` holds.
- Ensemble land mixing, dry-blind watering (−1k), grid-dropout mains, farsite / farring placement (−13k to −26k on fresh mains).

### Compiler / opening
- `nearanimals=6` (M&M placement order): G3 +1.2k and Local-LB +90 rating, but **lineage-only**: live continuations −106 /
  −66, hybrid level.
- Opening script (M&M's fixed crew script as binding fields): −615 / −202 on real opponents; −4.4k / −2.4k vs #1.
- earlydep, earlyforce, woolfirst, woolhires (forcing early delivery): moves nothing or −0.85 to −2.65k. Early wool is M&M's
  whole crew / layout pattern, not one route rule.
- More compiler effort (rounds 12, scen 32): −0.55k. animalring / sitelife (lifetime site cost): small or negative.
- Wheat floor / crop swap toward M&M's wheat: −0.9k to −6.7k on our farm.
- `saleslots=4`: 31% vs #1 on the exact LB. Route-scoring micro-optimizations: ~0.

### Seller / market
- Holding stock overnight, dawn share, the "regime" night carry: −189 per game-day, −0.6 to −0.9k, −2.3k to −5k.
- Copying top-team sell-rate curves: −4 to −8k on our farm.
- Seller timing knobs `hourdisc`, `leader` (Stackelberg), `rival=1.5`: −1.1k to −2.1k in the lineage league.
- Lineage-weighted forecasters (fclin: 6x weight on our own games): led on the LB, dropped by user rule as lineage fit.
- A team-identity forecaster (FCT7): deviance 0.21 -> 0.08 but ~0 value in games.

### General lessons from failures
- A gain on a fixed / replayed opponent is necessary, not sufficient: reacting opponents can take it back.
- When an arm collapses with land < 4, look for unspent day-0 cash before blaming the land rule.
- Knobs tuned under a wrong engine model flip sign once the model is fixed. Fix the model first.
- If one bed says +1k and another says −1k, find out which opponent behaviour each bed assumes before averaging them.
