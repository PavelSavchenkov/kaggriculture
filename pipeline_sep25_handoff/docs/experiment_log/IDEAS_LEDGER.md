# Ideas ledger

Sources: two external reviews of BC_issues_sep25.md (ChatGPT shares 6ab5b8b7 "BC Fix
Strategies" and 6ab5b8d8 "BC training fixes research", read in full on Sep 25) plus
this session's own findings. Status: tested / partly tested / open. Evidence cited by
those reviews is recorded as their claim, not verified here.

## Data and teachers (multimodality)

1. **Teacher/style conditioning.** Team or submission-family embedding (32-64 D), or
   small per-teacher adapters/heads on a shared trunk; one style fixed for a whole
   game; at inference try the 3-6 strongest style pins and keep the best by full
   games. Claimed precedents: Lux 2021 4th (separate final head per expert), Lux S3
   8th (agent-ID conditioning), Orbit Wars 2026 conditional BC (teacher one-hot +
   recency). Status: open. Needs submission IDs per perspective (being collected).
2. **Broad pretrain -> fine-tune on 1-3 strong recent submissions** (low LR 1e-4 or
   3e-5, early stop by full games). Precedents claimed: Lux 2021 6th, Lux S3 4th/9th,
   VPT. Status: partly tested. v9 -> DECEM fine-tune: 40/64 vs each of the Local-LB
   top 3 (vs 30-32/64) and 54/64 vs agent_sep23; DSM neutral; Mother-Goose worse.
   Team data is small (4-6k dawns), fine-tune overfits after ~500 steps.
3. **Measure cloneability per source**, not only leaderboard rank: quick clone per
   source, closed-loop test. Status: partly (three teams above).
4. **Recency and quality weighting** w = w_quality x w_recency; label every
   perspective with team, submission, replay time, rating at replay time, engine
   version. Status: in progress (metadata collection).
5. **Latent strategy / mixture of experts / BeT or VQ-BeT-style mode tokens**: start
   with known teacher IDs, then cluster whole-game behaviour, then learned z. Status:
   open (after 1).

## Architecture and encoding

6. **Set Transformer over group tokens** ([GLOBAL], [STYLE], [HISTORY], crop and animal
   group tokens with type and fixed-mask embeddings; 2-4 blocks, d 192-256, 4-8
   heads) replacing mean/max/sum pooling; each group decoded from its contextual
   token. Status: open. (Our grid CNN did not help; reviews rate grid scaling low.)
7. **Temporal memory**: GRU over the last 4-8 dawn summaries (state, own previous
   DayIntent, compiler trims/failures, actual outcomes, inferred opponent flow);
   auxiliary opponent heads (tomorrow's supply, expansion, sales). Status: open.
8. **Compiler-aware strategic features**: mandatory service work, wheat shortfall
   after mandatory feeds, expected harvest/collection inflow, shed slack after
   returns, free buildable tiles, work needed for +1/+5/+10 entities, minimum hires,
   cash after mandatory costs, affordable cow/sheep, units lost by waiting 1-2 days,
   market recovery and opponent ready-to-sell estimates; threshold booleans. Status:
   partly (v2 capacity, calendars, per-group value/deadline/distance).
9. **Numerical feature embeddings** (piecewise-linear / periodic) for day, cash,
   prices, counts, days-to-production, distances. Status: open.
10. **Auxiliary economic heads** trained from engine replay of the label: end-of-day
    shed, night overflow, wheat and fertilizer use, worker demand, harvested and
    collected units, decay, escapes, end cash, free tiles tomorrow. Status: open.
11. **Retrieval-augmented BC** (TabR-style): retrieve 8-32 similar expert dawns
    (same style first, never same episode) as attention context. Status: open.

## Training objective

12. **Remove Stage-A teacher forcing**: group heads trained on model-predicted,
    detached whole-farm values (cached predictions, then joint fine-tune; occasional
    sampling). Scheduled sampling only as a heuristic ablation (statistically
    inconsistent). Better: model new counts jointly with fields that depend on them.
    Status: open (v6-v9 still teacher-force Stage A and new-group sizes).
13. **Constrained structured likelihood (CRF-like)**: per group, score S(k) = sum_j
    l_j(k_j), loss -S(y) + logsumexp over legal compositions, computed with the same
    DP used for MAP decoding. Status: open (v7/v9 train marginals, decode by DP).
14. **Ordinal / EMD auxiliary loss** (0.05-0.3 x CDF loss; keep CE; CORAL as an
    alternative). Status: open.
15. **Loss balancing**: per-family normalized means (portfolio, one-shot, ongoing,
    animal, auxiliary), then GradNorm; PCGrad/CAGrad if gradient cosines conflict.
    Status: open (one-shot options dominate the loss).
16. **Upweight key decisions / stratified sampling**: land, animal expansion, big
    clears, strategy transitions, shed > 80, crops near decay, day 25+; focal or
    class-balanced losses only selectively (natural frequency carries policy
    information). Status: open.
17. **Counterfactual ranking / energy loss and advantage-weighted BC** (SPEN, implicit
    BC, AWR/IQL-style weights from compiled rollouts of perturbed intents). Status:
    open (needs an intent evaluator).

## Decoding and inference

18. **Farm-level autoregressive decoder with a running resource ledger** (free tiles,
    tiles cleared, shed headroom, wheat and fertilizer need, expected inflow,
    workload, committed cash); order: land -> survival/retention -> harvest, collect,
    clear -> care, fertilize -> new entities and reserves -> new-group fields; hard
    masks; train also on corrupted/model-generated ledgers, loss only where the target
    is still feasible. The Sep 23 BC had a running summary and free-tile mask.
    Status: open.
19. **Exact composition for whole-farm counts** (new crops sum to a total by
    remaining-count autoregression or DP; couple new animals with reserves).
    Status: partly (total x shares with largest remainder).
20. **Global k-best constrained decoding** (DP/beam/CP-SAT) returning top-K legal
    intents with scores. Status: open. Diagnostic E8: offline, is any of the top 16
    candidates much better than MAP? Separates representation from decoding.
21. **Economic reranker V(s, I)** trained on compiled continuations of expert, BC and
    perturbed intents; inference: K candidates -> reranker -> compile the best (keep
    compile latency within budget). Status: open.
22. **Economic Bayes decoding of counts**: k = argmin sum_y p(y) C(k, y) with
    asymmetric costs (0 instead of 20 is catastrophic; 18 is fine); calibrate
    temperature first. Status: partly (median instead of argmax).
23. **Ensembles (5 seeds) + uncertainty-triggered search** (entropy, seed/style
    disagreement, best-vs-second gap). Status: open.

## Closed-loop data

24. **Expert iteration / search-distilled BC**: play the current BC, and at on-policy
    dawns search nearby legal intents with compiler + rollouts against a reactive
    opponent pool; add the best as labels; retrain (DAgger with a search oracle).
    Status: open.
25. **DART-style perturbations**: perturb earlier intents or state (one fewer crop,
    cash or shed changes), relabel by search, never by the stale original label.
    Status: open.
26. **BC -> conservative RL** (league of frozen strong agents, KL to BC or a delayed
    teacher) as a final stage; precedents claimed: Orbit Wars 2nd/5th/11th.
    Status: open, last.

## Evaluation

27. Promotion hierarchy: legality -> field errors -> compiler feasibility and trims ->
    economic regret proxy -> paired full games vs a fixed panel -> larger tournament;
    cluster by seed (seats mirror); replay continuations diagnostic only.
    Status: adopted except the regret proxy.
28. Diagnostic factorial: A baseline, B + style, C + Stage-A self-conditioning,
    D + set attention, E + structured NLL. Status: open.

## Deprioritized by both reviews

Bigger MLP, raw grid CNN scaling, diffusion/flow policies, blindly longer training,
a very large transformer, scalar count regression.

## This session's own entries (tested)

- Compiler trim fallback for unfunded early days: 14/64 -> 42/64 vs agent_sep23.
- Count categoricals instead of per-member fractions; sequential decoders with
  remaining counts; marginal heads + exact MAP DP (DP on non-marginal heads fails).
- Cash protection (runtime guard + next-dawn reserve with sellable assets): fixes
  total collapse vs teammate_shoprouter (0/64 -> 64/64) at a cost vs agent_sep23
  (61 -> 48/64); the opening trade-off between aggression and safety is unresolved.

## Sep 25 (24 h goal) - evidence and priorities

Evidence so far:
- Uniform decision pushes all lose (-0.6k to -6.3k per game), but the per-game best of 10
  push settings wins 64/64 (+20.0k vs +8.0k): outcomes are decided by state-dependent
  network choices. Test: per-dawn search with exact opponent copies (tools/search_games).
- Local-LB = ~6-7 distinct behaviours (identical games: shiiin9 = arsgorynich,
  arlene-wheat = ahmed, haideptry = sunil = arlene-idle). v11 wins 81-88% vs each.
- Losses vs the top family: fewer animals mid-game (13-14 vs 16 in wins; rival 17).
  Remaining trims (4.9% of days) are mostly day 3-5 cash limits; trims drop animals first.
- Frozen top-team replays are weak opponents (their recorded orders fail once the market
  differs: $85.5k vs $110k originally); diagnostic only.

Queue (ROI order):
1. [running] v12: 13x data + strength/recency conditioning + site features; v12_aug adds
   dihedral grid augmentation. Gate: panel (6 Local-LB behaviours x 64 games + 8 extra
   agents + 7 C++ + replays), paired vs v11.
2. [running] Search headroom (king_rc4, ahmed_v25; days 0-12; 7 candidates). If large:
   expert iteration - search-improved games vs a diverse cloneable league (C++ agents +
   BC zoo) as extra training data (DAgger-like, no single opponent).
3. [ready] Trim order: crops before animals (DC10_TRIM_CROPS_FIRST).
4. BC zoo from v12: fine-tunes on DSM, Majkel, Mother-Goose, M&M, SpaTaro, Crop Dusta;
   used as league opponents and as candidates.
5. Inference-time strength sweep (BC_OPUS_STRENGTH) - does conditioning steer quality?
6. Sales audit vs top players (sale hours, prices, holding) - earlier +2.2k on agent_sep23.
7. Heavy-day routing capacity (compiler) - large late days still trim/fall back.

## Sep 25 04:00 - deviations from top players and data-sampling ideas

Deviations (our v11 games vs the top-10 Kaggle teams' own games):
- Animals from day 13: 13 vs 20 (geese 1.8 vs 5.0, sheep 3.7 vs 6.6, cows 7.8 vs 8.8);
  land 2.9 vs 3.4; plants equal. Units sold 1,408 vs 2,056 (wheat 349 vs 711, eggs 59
  vs 163); our sale price is better (1.01 vs 0.88 of base) and output per animal-day
  is higher. The gap is farm composition, not animal service or selling.
- Teacher-forced decoding on top-6 teams' held-out dawns matches their totals (new
  animals 1.6/1.6 per day on days 0-9): the network is unbiased on expert states; the
  gap arises in closed loop.
- Closed loop: day 6 we buy ~1 goose and ~1 cow fewer than top players; our cash on
  days 1-2 is $530-570 (top $7-29: we keep cash unspent; the next-dawn reserve fails the
  day-0 plan) and we buy the 2nd quadrant a day earlier (cash $205 at day 6 vs $840).
  Trims (day 3-5) remove animals first (mostly cows).
  Tests queued: no next-dawn reserve, no runtime cash guard, crops-first trims.

Data-sampling ideas (test those that move full games):
1. Relative-strength filter/weights (min strength 0; or weight = exp(strength/100)).
2. Recency filter or exponential decay by replay date (meta drifts; older = weaker).
3. Winners only, or weights by episode margin (sigmoid(margin / 5,000)).
4. Opponent strength: keep/upweight games against strong opponents (their decisions
   under pressure are what the Local-LB and Kaggle LB test).
5. Team-balanced sampling (cap prolific teams such as Majkel/SpaTaro/ymg_aq) vs
   oversampling the current top 6.
6. Submission-balanced sampling (one converged submission can dominate a team).
7. Day-stratified sampling: oversample days 0-12 (investment decisions, where our
   closed-loop deviation starts) or days with animal/land purchases.
8. Two-stage: broad pretrain -> short fine-tune on strong + recent data (vs one stage).
9. Loss-based hard-example resampling (days the current model decodes worst).
10. Exclude broken perspectives (timeouts, collapsed farms, final money far below
    the submission's norm).
11. Search-improved self-play games (expert iteration) as extra labels.
12. Training length: 10k vs 40k steps (tested in d_all10k).

## Sep 25 05:00 - results since 04:00

Tested (paired, screens unless noted):
- Training length: 10k vs 40k steps (all data): -4.8k [-8.6k, -1.0k], Local-LB 57% vs
  88%. Validation loss tracks game strength. -> long runs (v13 60k; v13_w384 120k).
- Data filters (10k steps each): strength >= 0 only (8.5% of dawns): -11.3k vs all data
  (Local-LB 25%). More data beats quality filtering; condition instead of filtering.
  Recent-only and winners-only: running.
- Grid augmentation (full panel): +0.8k [-1.6k, +3.0k], wins 92% -> 95%. Keep.
- Style: style 0 on v5 = average unknown team (-6.6k). Fixed in train.py --style-v2.
- Strength input: +150 best for Local-LB (-100: -2.5k LB, +4.2k C++; +300: -2.8k LB).
- Openings (v12, days 0-5 team style, pooled after): DSM +5.6k [-0.8k, +12.7k] (LB 81%
  -> 97%); M&M -3.2k (LB 34%). Whole-game pins: DSM +0.5k, M&M -4.5k. The opening is a
  large lever; DECEM / Mother-Goose / Majkel / Vadim openings running; DSM opening on
  the full panel.
- League (C++, v12 vs zoo fine-tunes): DSM 24/32, DECEM 20/32, Mother-Goose 28/32,
  Majkel 13/32 (Majkel fine-tune beats v12; screening it vs Local-LB).
- Compiler: reserve needed mid-game on v12 (no-reserve collapses seed 706); v12's day-0
  plans never hit the reserve (v11's always did). No cash guard -0.8k. Crops-first trims
  -3.4k.

Next ideas:
- Opening length sweep (days 0-3 / 0-8 / 0-12) with the best opening style.
- Opening model = best zoo fine-tune (e.g. zoo_dsm or zoo_majkel) instead of a style pin.
- Per-phase selection generally: style/model per day range chosen by screens.
- v14: longest run + augmentation + style v2 (+ opening pin).

## Sep 25 09:55

- Openings, full Local-LB panel (384 games, vs v12 88%): Vadim opening (days 0-5) 94%
  (+5.7 pts [-0.5, +13.5]; +12.5 pts [+3.1, +25.0] vs ahmed and vs arsgorynich), DSM
  opening 93%. Headline candidate: v12_cond + Vadim opening. Opening length sweep running.
- Ensemble of whole-farm heads (v12 + Majkel + Vadim fine-tunes): -1.0k, -1.6 pts (dropped).
- Width 384 at step 29.5k (valid 17.01 < 17.23): neutral in games (-3.9 pts, +0.7k);
  judge after its schedule ends (v13_w384b).
- Search transfer: a mismatched opponent model keeps ~1/4 of the exact-copy gain
  (+9.4k, 88% -> 100%, 16 games). Expert iteration data generation started
  (6 opponent pairs, rollout model != real opponent, days 0-6).
Queue (one training at a time): v13_w384b -> v14 all data (66k perspectives) ->
expert-iteration fine-tune (human + search games, search games upweighted).

## Sep 25 12:30 - day compiler: weak points re-ranked from data (best agent's 384 Local-LB games)

Done / tested:
- Speed: failed market-return re-solves were ~75% of compile time (ascending hire scan x
  ~40-70 route heuristics per level). Feasibility-first hire search (portfolio at the max
  affordable workforce, down to the scan's first level, canonical re-search, old narrow
  minimization) is decision-identical on 240 recorded days + 16 games, -33% time: default,
  shipped to the Local-LB PR. DC10_HIRE_SCAN=1 restores the scan.
- Rejected: route-scoring micro-optimizations (exact but ~0 gain); memoizing reorder (2% repeats);
  timing-capped market targets (-306 [-2.0k, +1.4k], wins -3.5 pts); sale-value cap on extra
  hires (with the timing cap and the first feasibility-first version: -1.2k [-5.1k, +1.2k]).

Open, ranked by frequency x plausible value:
1. Intent over budget on investment days (day 0 all games, day 6 94%, day 7/9 ~50%): the
   network decodes entity counts independently; the compiler repairs by price (drop the most
   expensive entity) or drops land first. Arms running: DC10_TRIM_MODEL (decoder's
   likelihood-per-dollar trim order), DC10_LAND_FIRST (trim entities before dropping land).
   Later: budget-aware decoding (most likely affordable combination).
2. Cash-flow timing: purchases at hours 0-2 before same-day sales ("unit action fails at hour
   2" under both forecasts); financing variants often fail to route.
3. Market returns all-or-nothing on heavy days (29% of market attempts fail -> near-zero returns);
   the capped target did not win games, so the value is smaller than it looked.

## Sep 25 13:00 - evaluation correction and results on fresh seeds

- Seeds 700-731 selected the Vadim opening, so any change of days 0-9 regresses there (winner's
  curse): the baseline wins 94% / +10.9k on 700-731 but 88% / +8.4k on fresh seeds 1200-1231.
  All A/B from now on: fresh seeds, paired against reports/panel/fresh_base.
- DC10_TRIM_MODEL (decoder likelihood-per-dollar trim order), fresh seeds, 246 games: -1.1k
  [-2.6k, +0.2k], wins -1.6 pts. Rejected (price order stays; crops-first also lost earlier).
- DC10_LAND_FIRST: identical plans to the baseline on 250 fresh games (the baseline already trims
  at level 0 with the land kept when entities would be lost); fallback level 1 on days 7/9 was not
  a land drop in most cases (intent without land). Rejected as a no-op.
- New data source: xishengfeng/kaggriculture-replay-db (67k episodes Sep 7-25 with seeds and
  actions, all ratings; 54k not in our corpus, 39k with a >= 2700 player). Conversion verified:
  identical actions and all 720 parity hashes vs official traces; daily snapshots match.

## Sep 25 13:10 - flexibility test (reports/flex, 48 fresh games vs arsgorynich/ahmed/wzhengbiao)

- Per-game best of 8 settings flips 12/12 base losses for BOTH network pushes and compiler variants
  (mean best +18.0k vs +18.2k; base +9.7k): losses are fragile re-rolls, so whole-game best-of
  does not separate the parts. Per-dawn search (state-dependent) remains the RL evidence (+9.4k
  with a wrong opponent model, earlier session).
- Single settings (paired margin): crops-first trims +4.4k (48/48 wins vs 36/48), crop quantile
  0.35 +3.4k, DSM opening +1.8k (48/48), land bias +2 +0.8k, rival stock from day 20 +0.4k;
  animal quantile 0.35 -2.6k, sampled decoding -3.4k, no next-dawn reserve -22.2k.
- Consistent with the Local-LB loss analysis: in losses the network asks for fewer animals on days
  10-15 (3.9 vs 5.1) and ends with 12-14 animals vs the rival's 17.
- Next: broad unseen panel (all 10 Local-LB agents, seeds 1300-1323) for crops-first, crop 0.35,
  DSM opening, v16_db; holdout 1500-1523 for the winner.

## Sep 25 12:50 - reprioritised (evidence from the broad unseen panel)

1. [running] Herd funding fix: X1 (day-0 crops-first trims + no day-0 reserve) on lb10 seeds
   1300-1311: +7.6k [+1.9k, +14.5k], wins 83% -> 100% (96 games). Confirmation on 1312-1323
   running; farm-aware reserve (DC10_RESERVE_FARM: fertilizer and held product on animals count
   as tomorrow's liquidity, all days) running as the general version.
2. Seed dependence: all baseline losses are egg-shop games (BAKERY/BRUNCH by day 12) with a small
   sheep/cow herd; check whether X1 removes them.
3. [training] v16_db: 2.2x data (replay DB). Evaluate with the winning compiler.
4. Opening re-screen under the fixed compiler (Vadim was chosen while day 0 always lost a sheep).
5. Melon race (X2): no gain on top of X1 in this panel (X4 +6.1k vs X1 +7.6k); retest later.
6. Wages/returns in one route objective (other report C), after 1-4.

## Sep 25 13:00 - RL framing (for a future session)

- The network makes 30 decisions per game (one DayIntent per dawn); everything else is the
  deterministic compiler. A game costs ~5 s CPU (X1 compiler), so ~40k games/day on this machine:
  short-horizon RL is feasible. Candidate: KL-regularised policy gradient (REINFORCE with a value
  baseline, KL to the BC policy as in AlphaStar / Orbit Wars BC+RL) on the whole-farm heads
  (new-entity totals, shares, land) first, league of C++ agents + BC zoo + frozen past selves,
  reward = win (+ small margin term). Evidence of headroom: per-dawn search (+9.4k with a wrong
  opponent model; exact-copy numbers pending with the X1 agent in reports/search_space).
- Expert iteration (search-distilled BC) is the cheaper first step; data generation must be redone
  with the X1 compiler (the herd fix changes the states).

## Sep 25 15:40 - re-ranked after the market slot finding

Engine fact (fast_game_engine/sim.hpp market phase): order slot k of both players is processed
together, one unit each per round at the same quote, and prices refresh only after the slot. A
sale listed in an earlier slot than the opponent's sale of the same product gets the higher
prices. Our compiler (and herd/robust/vadim) listed sales wheat -> fertilizer, so high-value
products (melon, milk, wool, strawberry) went last.

1. [testing] Sales by descending price (sell_by_price): h2h vs herd 36/40, +9.2k (control with
   the old order: mirror, +26); vs robust 40/40, +11.4k. Gates: Local-LB panel, C++, replays, zoo.
2. [next] Contested-product ordering beyond price: units x price slope x expected opponent units
   that hour (the opponent forecast already exists: rival[h][p]); and let the sale DP assume we
   sell before the opponent when our slot is earlier.
3. [running] Value model V(s) decision test (exact-copy rollouts of the 7 pushes, days 0-9).
   If argmax V beats the baseline candidate: propose-and-evaluate at inference.
4. Melon race on top of sell_by_price (c2csp) - measured with the fixed harness.
5. Expert iteration fine-tune (data: reports/expert_iter_robust).
6. Night inventory / compiler horizon across the night (other session's evidence).
7. RL on whole-farm heads (after 3 and 5).
Dropped: more replay DB data for BC (v16, v18 both worse or equal).

## Sep 25 16:40 - re-ranked (agent: wages = slot sales + wage-aware returns, Local-LB #1 candidate)

Findings since 15:40: slot priority (+9-11k vs our old agents), wage-aware returns (+0.9k exact
Local-LB, +1.8k third-party panel), seed lottery (same recipe, 8-32 vs v12), replay-trained value
model fails decision ranking, fertilizer-value / full-care / race / network trims neutral or worse
on top of slots.

1. [running] Inference settings under the new compiler (opening style/length, strength): C++
   mirror vs wages; confirm winners on fresh seeds + Local-LB panel.
2. [running] Network selection: v12-recipe seed replicates screened by mirror games (valid loss does
   not predict strength). Cheap GPU use; one training at a time.
3. Marginal-hire economics for the base work (the wage gap vs JJ's PPO agent is halved: $273 vs $223
   per day; 31% of days still 12+ hires): drop or defer the lowest-value optional work (clears,
   fertilize, water-for-production, far collections) when it forces the 12th/13th hire, valued the
   same way as returns.
4. On-policy value model (paused data generation; resume when CPU is free) -> decision test ->
   propose-and-evaluate.
5. Night inventory / dawn sell-down (other session's gap 3), melon race variants: later.
Dropped: more BC data (DB), wider nets, fertilizer value rule, full care.

## Sep 25 17:15 - re-ranked (wages merged into the Local-LB; next candidate = DSM opening + fixes)

Adopted since 16:40: DSM opening (82-38 vs Vadim), hire_cap_stock (rescues crunch days, else exact).
Evidence of the main remaining gaps:
- Herd size (replay gate: top players 18 animals by day 12, we stop at 14 with $10-15k idle cash).
  Probes: q_animal 0.7 on days 8-14 45-27-8 (+0.5k) over 80 games; budget-aware reach (keep the
  higher quantile only if the plan still compiles in full) with 0.8/0.7/0.6 24-16 (-0.4k); reach with
  0.7 only running. Principled next step if confirmed: reach inside the decoder (most likely larger
  herd that the funding envelope allows).
- Worker efficiency (other session, labour ledger): 0.51 vs 0.57 actions per worker-turn, more walking,
  twice the idle turns = the whole remaining wage gap. Route construction (gap 4); their probes run.
- Night inventory (gap 3): no winning design yet.
Closed today: style schedules beyond the DSM opening, opening models, ensembles, v12 seeds 1-4 (one
equal, three weaker), strong-player fine-tune, value models, load leveling, deadline 22, fertilizer
rule, full care.

## Sep 25 17:50 - agent "forecast" pushed; what worked today (vs the previous candidate, mirror)

| Change | Evidence | Status |
|---|---|---|
| Sales by revenue at stake (slot priority) | 39/40 vs old order | in all agents since slots |
| Wage-aware same-day returns | +0.9k exact Local-LB, 28-12 vs slots | since wages |
| DSM opening (days 0-5) | 99-61 vs Vadim (5 sets) | forecast |
| Herd reach (larger animal quantile if fully funded, days 6-14) | 86-28-6 | forecast |
| Collect all fertilizer (other session) | 88-32 | forecast |
| Opponent supply forecast from its visible farm (other session) | 101-19 | forecast |
| Hire cap counts hour-0 sellable stock | neutral except crunch days | forecast |
Rejected today: race, network trims, fertilizer value, full care, slack care, load leveling,
deadline 22, strength 100 with DSM, other opening/mid-game styles, zoo opening models, ensembles,
seed replicates (none better than v12), value models, reach beyond day 14.
Open: route search (neutral on 1300), racedp (testing), night inventory, what top-team replays still
do better (replay gate running for forecast).

## Sep 25 18:50 - forecast / timing ideas

- Measured (tools/forecast_audit): against varied opponents the hourly timing of their sales is the
  larger error (melon 65%, wool 56%, milk 43% of units in the wrong 6-hour bucket); against our own
  lineage the blend is already good (mirror cannot show forecast gains; judge on the C++ / Local-LB
  third-party panels).
- In test: online forecast selection, intraday conditioning, the other session's prior timing + early
  race delivery (fin_W).
- Next (routing, from the other session): melon sale timing is limited by the first harvest-return
  trip (hires act from hour 1, melons ~4 tiles from the shed -> first sale ~hour 9-10, opponents start
  at 9): plant melons nearer the shed on day 0, or end the farmer's day next to tomorrow's ripe melons.
