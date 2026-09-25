# Lineage: how the results were reached

Chronological, with the evidence that decided each step. Times are BST (UTC+1) on Sep 24-25.
The full audit log is `docs/experiment_log/PROGRESS.md`; the ranked idea list with every tested
and rejected idea is `docs/experiment_log/IDEAS_LEDGER.md`.

## 0. Starting point

- Network + compiler split and its contracts: `docs/design/day_intent.md` (what the network must
  emit, fixed-value rules, exact label conversion with a coverage gate) and
  `docs/design/day_compiler.md` (what the compiler must decide and how to validate it).
- Day compiler copied from `experiments/v10/sep24_day_compiler_opus` (hashes in
  `experiment/data/COMPILER_SOURCE.sha256`); route solver from the repository's `day_policy`
  (local copy in `experiment/day_policy_local/`).
- Opponent to beat: `agent_sep23` (committed in `agent_sep23/`), the previous strongest BC agent.

## 1. Network v1 -> v9 (Sep 24 22:10 - Sep 25 00:10): output design and one compiler bug

Full games vs `agent_sep23`, seeds 600-631 both seats (64 games) unless noted.

| Version | Change | Wins | Mean margin |
|---|---|---|---|
| v1 | MLP, per-type argmax counts, group fields as one per-member probability | 0/32 | -32.2k |
| v1 median | median instead of argmax for whole-farm counts | 6/32 | -9.9k |
| v2 | inputs v2 (product calendars, free tiles, affordability, per-group value/deadline/shed distance), width 256 | 14/64 | -18.1k |
| v2 + trim | **compiler**: unfunded day trims new entities one unit at a time instead of dropping all | 42/64 | +4.9k |
| v5 | 101-way count categoricals per field, greedy | 52/64 | +13.0k |
| v5 + DP | exact MAP DP over heads trained sequentially (untrained logits) | 0/64 | -133.8k |
| v6 | sequential decoder with assigned/remaining counts as inputs (as in the Sep 23 BC) | 49/64 | +10.3k |
| v7 | marginal count heads (0..size), exact MAP DP under partition constraints | 58/64 | +15.7k |
| v8 | v6 + 1,510 fresh perspectives | 54/64 | +16.4k |
| v9 | v7 + fresh data | 64/64 | +20.6k |

v9 on fresh seeds 700-731: 61/64 (+19.2k). Lessons: argmax of a spread count distribution says 0
(under-planting); a rounded per-member probability blurs whole-group decisions (experts act on the
whole group 84-98% of the time); DP decoding needs marginally trained heads. The teacher-forced
decode check (`tools/decode_eval.cpp`) caught the DP failure at once (86% member mismatch).

## 2. Robustness, style, data (Sep 25 00:10 - 05:30)

- Total collapse vs `teammate_shoprouter` (day 0 ended at $0; no hires, everything died): runtime cash
  guard + next-dawn cash reserve counting sellable assets: 0/64 -> 64/64.
- v11: team style one-hot (10% dropout). Style index 0 turned out to mean "average unknown team";
  the all-zero style input works. Local-LB top 3 (128 games each): 66-75% wins.
- Compiler determinism: two 2 s wall-clock cut-offs made results depend on machine load; replaced by
  fixed counts. Site capacity enforced in validation and decoding (the network asked for 19 new crops
  on 18 free tiles; one day took 20 s). Paired vs the old compiler: +1.8k to +2.2k per game on all three
  Local-LB top agents; trimmed days 13.7% -> 4.9%.
- Data 13x: official daily episode datasets (every episode from Aug 16 replays exactly in our engine;
  earlier rules differ), Meta Kaggle ratings, API listings -> corpus v5: 48,465 perspectives, 1.45M
  days, coverage gate 0 failures. Strength = submission rating minus the ladder's daily top-episode
  90th percentile on its date (ratings drift ~300 points over weeks), used as an input, not a filter.
- **v12_cond** (v5 data, strength + recency conditioning, site features, 40k steps, 46 min on an RTX
  5090): full panel of 865 games vs v11: wins 87% -> 92%, +2.1k [+5, +4.5k]; Local-LB 79% -> 88%.
  v12 remains the network of every later agent.
- Data levers at 10k steps vs all data: strength >= 0 only -11.3k (Local-LB 25%), winners only
  -4.3k, recent only -2.6k; 10k vs 40k steps -4.8k (Local-LB 57% vs 88%). More data and longer
  training win; condition instead of filtering.
- Opening pins (days 0-5 decoded with one team's style, pooled after): Vadim +9.4, DSM +7.8, Majkel
  +3.9, Mother-Goose +3.1, DECEM -1.6, M&M about -25 win points (mini screens). Whole-game pins are
  worse. 6 days beat 3, 9 and 12.
- Per-dawn search with exact opponent copies (7 day-level pushes, full-game rollouts, days 0-12):
  king_rc4 +11.9k -> +37.2k; with a wrong opponent model about a quarter of the gain survives
  (+9.4k, wins 88% -> 100%). Large headroom in the network's decisions; test-time search does not fit
  the 60 s overage, so this points to expert iteration or RL.
- 05:30-09:25: the session died (out of memory: three trainings at once). One training at a time since.

## 3. Local-LB agents (Sep 25 10:00 - 17:45)

| # | Local-LB id | Model folder | What changed | Evidence |
|---|---|---|---|---|
| 1 | `pavel-bc-opus-v12-vadim` | cand_v12_vadim6 | v12 + Vadim opening ("6 8"); feasibility-first hire search (decision-identical, -33% compile time) | Local-LB 333/400 (83%), Elo 1892 |
| 2 | `pavel-bc-opus-v12-herd` | cand_v12_vadim6 (X1 compiler) | day 0 trims crops before animals; next-dawn reserve from day 1 | unseen seeds +6.6k, wins 86% -> 97.5% (480 games); exact Local-LB replay 398/400; Local-LB #1 (Elo 2283) |
| 3 | `pavel-bc-opus-v12-robust` | cand_v12_vadim6 (trim0) | herd fix but reserve kept on day 0: X1 collapsed 3 of 145 frozen replays | 0 collapses, replays 91% (X1 90%); gates +5.0k [+1.4k, +8.5k] vs vadim; the Kaggle submission |
| 4 | `pavel-bc-opus-v12-slots` | cand_so2 | herd rules + **recovery level** + **sales by revenue at stake** (`sell_order 2`) | mirror 39/40 vs old order (+11.6k); exact replay 399/400 -> Local-LB #1 399-1-0 (rating 2802) |
| 5 | `pavel-bc-opus-v12-wages` | cand_so2rw | + `return_wages 1`: same-day market returns only if the sale gain covers the extra Fibonacci wages | exact replay +$901 per game vs slots (270 better, 116 worse); 28/40 vs slots; merged (PR #184) |
| 6 | `pavel-bc-opus-v12-forecast` | fin_I | DSM opening "6 7"; herd reach days 6-14; hire cap counts shed stock; collect all fertilizer; visible-supply forecast | vs wages 164-36 (5 seed sets); current roster 137/140; all 69 agents 962/966; replays 143/145 |

Steps in detail:

- **Compile speed (vadim)**: ~75% of compile time was failed market-return re-solves (the ascending
  hire scan ran ~40-70 route heuristics at every hire level before failing). Feasibility-first: search
  the portfolio at the largest affordable workforce, descend with the winning configuration to the
  scan's first level, re-search canonically there. Decision-identical on 240 recorded days and 16
  games. Route-scoring micro-optimizations gave ~0.
- **Local-LB result explained (13:00)**: PR 1 scored 83% vs 94% locally. Not the host: replaying the
  Local-LB's own seeds reproduced its W-L for all 10 opponents. The local panel had used the seeds
  that selected the opening (winner's curse) and omitted two agents wrongly assumed identical to
  others. From then on: unseen seeds for selection and the exact Local-LB seeds before pushing.
- **Seed dependence (herd)**: all 37 baseline losses on the broad panel were games with an egg shop
  (BAKERY or BRUNCH_SPOT) unlocked by day 12 while we held 2-3 sheep vs the rival's 4-6. Cause (found
  with the companion study): day 0's plan was ~$50-80 over budget and the trim removed the most
  expensive new entity, a $500 sheep, in 100% of games, because the reserve ignored overnight
  fertilizer. Day-0 crops-first trims fixed it; all 20 egg-shop losses turned into wins.
- **Collapse mechanism (robust -> recovery)**: in the 3 collapses a day 7-8 cash crunch sent the ladder
  to survival-only, which disabled every harvest and collection and capped feed by morning cash; all
  12 animals were lost. Recovery level: survival keeps the network's harvests and collections and
  feeds from cash plus 80% of that income. The 3 games went from -119k/-79k/-95k to +3k/+2k/+21k.
- **Harness bug (15:25)**: older Local-LB packages of this agent set `BC_OPUS_MODEL` at import; our
  bridge reads it when a game starts, so in head-to-heads our challenger silently loaded their
  folder (no compiler sidecar). Every sidecar head-to-head before 15:25 was invalid.
  `scripts/lb_play.py` now sets our path only around our `opus_new`.
- **Order slots (slots)**: engine fact (`fast_game_engine/sim.hpp` market phase): order slot k of both
  players is processed together, one unit each per round at the same quote; prices refresh after the
  slot. Our compiler listed sales in product-index order (wheat first, fertilizer last), so milk, wool,
  strawberries and melons went last. Price order won 36/40 (+9.2k) vs herd; revenue-at-stake order
  (price drop our quantity causes x units) 36/40 (+3.2k) vs price order. No cost against third-party
  agents (they sell at other hours).
- **Wages**: JJ's PPO agent held 11 hires a day ($223/day) while we used 12-13 hires on 39% of
  mid-game days ($316/day), mostly same-day market returns added on top of the base route. The
  companion's version of this idea (per-product gain vs all wages) had lost -1.8k; comparing the sale
  DP's gain with only the marginal wages of the added hires won.
- **DSM opening (forecast)**: under the new compiler DSM beat Vadim 25-15, 31-9, 26-14 but lost 17-23
  on the Local-LB seeds of the wages pairing: 99-61 overall. Kept, with the rule to adopt only after
  >= 4 seed sets including the Local-LB seeds.
- **Herd reach (forecast)**: frozen replays showed top players reach 18 animals by day 12 while we stop
  at 14 with $10-15k idle cash. On days 6-14, when the day compiles in full, try the network's 0.8,
  0.7, 0.6 quantiles of the new-animal total and keep the first that also compiles in full (funded,
  next-dawn reserve kept): 86-28-6 vs DSM alone. A fixed quantile push was weaker (45-27-8).
- **Hire cap counts shed stock (forecast)**: a dawn with $14 cash and melons in the shed allowed only 5
  hires, nothing routed, the day fell to survival (mirror seed 1409, -14.4k). Counting 90% of the
  hour-0 sellable stock in the cap: -14.4k -> -2.9k there, identical games elsewhere.
- **Collect all fertilizer (forecast)**: the compiler collected fertilizer only from animals with another
  job that day (23 uncollected units per Kaggle game vs top teams' 11). Found by the companion; 88-32.
- **Visible-supply forecast (forecast)**: the opponent sale forecast was its trailing 3-day hourly mean,
  which predicts nothing before its first sale and extrapolates dumping. Blend: melons = its visible
  ripe melons; other products half trailing, half visible ripe/held output. 101-19 on top of fin_D.

## 4. After forecast (17:45 - 20:00): diminishing returns, then a robustness fix

Every add-on measured 0 to +0.5k per game (50-60% of mirror games): blend weight 0.7/0.85, adaptive
sell-through forecast, inferred-stock forecast, crop reach, melon race, race_dp, route search mode 2.
Two directions still looked real:

- **Forecasts judged against varied opponents**: a linear opponent-sales model fitted on 600 recorded
  games cut held-out error 25-70% (the inferred shed stock is the missing predictor), but was neutral in
  the mirror: our lineage holds stock, other agents sell it daily, so one fixed model does not fit all.
  Online selection per product (blend or fitted, whichever erred less on this opponent over 3 days) is
  never the worst: C++ panel +658 [+126, +1,315] (`select`, fin_T).
- **Anticipation stack** (ported from the companion): strong-player hourly prior for melon/milk/wool,
  sale-DP-chosen delivery hours from hour 6, retries, melons sold on arrival. With selection =
  `anticipate` (fin_Y): mirror 36-4 then 29-11 (-0.7k), C++ panel +1.5k [-0.2k, +3.2k]; the companion
  measured the stack +3.0k [+1.0k, +5.0k] on 160 Local-LB games. Packaged as
  `submissions/sep26-bc-opus-v12-anticipate` (not in git).
- **A fragile rule in `forecast` (19:30)**: `anticipate` lost three mirror games by $11-28k. Ablation (one
  option removed at a time, 12 games, 76 s) pointed at the hourly prior, but the root cause (traced with
  `DC10_DAYLOG` and `farm_audit`) was herd reach: on day 6 it bought 6 geese and a cow with $1,090 on a
  plan funded only under the expected forecast; the prior's sale timing earned $488 less, dawn 7 had $20,
  no wheat, and 7 animals escaped. `forecast` itself has the same failure (mirror seeds 1302/1316: dawn 7
  with $17-31, 10 animals lost). Fix: decode key `reach_stress 1` (keep a larger herd only if also funded
  under the stress forecast): the three games went from -28.6k/-11.5k/-24.5k to +0.9k/+1.3k/-0.5k, and
  it is neutral elsewhere (fin_I_rs 6-5-9, 7-6-7).
- **Z+rs (20:00)**: `anticipate` without the hourly prior, with `reach_stress`: one-seat screen 19-1
  (+6.6k) and 18-2 (+4.0k) vs `forecast`; 37-23 head to head vs Y+rs; C++ panel +1.4k (222/224). The
  strongest candidate at the snapshot (`candidates/z_rs`), not yet confirmed on the Local-LB.
- **4th quadrant (20:00)**: the latest submissions of DSM, DECEM, Vadim and mtmr buy it in 95-100% of
  games, on day 10, funded by the day-10 melon sales, and grow mostly wheat on it (feed and cash crop);
  our network never buys it (none of v12's training data has it) and sits on $11k at dawn 11 with 3
  full quadrants. Forcing the land logit (`land_push`) and fine-tuning v12 on 1,851 top-15 Q4 games
  (`v12_q4ft`, style slot 31, used via the new `land_model` sidecar) both lost margin; the whole-model
  fine-tune degraded v12 even without Q4. With Q4 our own money rose ~+3.9k, but the shed overflowed
  (135 units destroyed vs 8-17 for the top agents), the purchase crowded out the day-12 herd reach, and
  a day-10 purchase cannot be funded (land at hour 0 only in practice). Conclusion: first the shed and
  sale capacity plan across the night (WEAKNESSES 2-3), then a Q4 specialist used only after buying.

## 5. Rejected (tested, lost or neutral)

| Idea | Result |
|---|---|
| Wider / other networks: v12_aug, v13_w384(b), v14_all, v15_w512, v16_db, v17, v18_dbstrong | all lose to v12 in games (section 7 of RESULTS.md) |
| Replay-DB data (xishengfeng/kaggriculture-replay-db, 41k verified episodes) | v16 -8.9k; strong part only (v18) 97/192 zoo |
| v12 recipe with other seeds (1-11) | 3-23 of 40 vs v12; v12 is a lucky draw |
| Fine-tune on strength >= 50; ensembles of whole-farm heads; zoo fine-tunes as opening models | -0.9k; 8-32; 9-31 to 16-24 |
| Style pins after day 5 (M&M, DSM, Vadim, DECEM, Goose, akmr, YumeNeko) | 26-14 then 17-23, 16-24 (noise) or worse |
| Strength input other than +150; strength 100 with DSM | 19-21, not additive |
| Crops-first trims on every day | -12.8k (day 0 only) |
| Network-ordered trims (least log-probability loss) / per dollar | +5.3k on Local-LB before slot sales (companion X10); on top of `slots` neutral (mirror 16-14-10); per dollar -1.4k. (A 6/22 head to head vs herd was measured before the harness fix and is invalid.) |
| No next-dawn reserve | -3.0k on v12 (seed 706 collapses); needed mid-game |
| Budget revision as a covering knapsack over units | works mechanically; the dollar shortfall is the wrong abstraction (day 6 fails on hour-0 cash timing) |
| Timing-capped market targets; sale-value cap on extra hires | -306, -1.2k |
| Full care / slack care / fertilizer-value rule | -1.3k / 9-31 / neutral |
| Load leveling (defer waitable work to save a hire) | 5-33: tiles bind, not wages (a harvest a day later delays the replant) |
| Market return deadline 22; hold factor 1.0; sell caps; fixed overnight stock | neutral; -3.8k; -23.6k; -13.2k (shed overflow) |
| Melon race with fixed deadline; copying the Kaggle public opening (2 sheep + 12 melons) | 41-39; -4.6k on Local-LB |
| Value model V(s) trained on replays (R^2 0.36-0.58 on days 3-9) | ranks our own decision alternatives worse than random (regret $3.4k vs baseline $1.4k); on-policy fine-tune did not fix it |
| 4th quadrant: forced on days 11-13 (companion), `land_push`, Q4 fine-tune (`v12_q4ft`) | -1.0k mirror / -1.0k frozen; below Z+rs; shed overflow (135 units destroyed) |
| Expert iteration v1 (5 opponent pairs) | only 239 changed days of 896: too little to train on; v2 started 18:30 |

## 6. Model-folder names

`cand_v12_vadim6` = vadim/herd/robust (compiler rules differed), `cand_so2` = slots, `cand_so2rw` =
wages, `fin_I` = forecast, `fin_P` = routes, `fin_T` = select, `fin_Y` = anticipate, `fin_Z_rs` =
z_rs (suffix `_rs` = `reach_stress 1`, `_sd` = stress_down, `Q4*` = 4th-quadrant probes). Other `fin_*`
letters: C DSM + hire cap stock; D C + herd reach; E Vadim + reach; F reach to day 18; G D + collect
all; H D + blend; Iv I with Vadim opening; J/J2/J3 route search 1/2/3; K slack care; L race_dp; M melon
race; N crop reach; Q forecast stock; R adaptive forecast; S fitted forecast; U intraday; V select +
intraday; W anticipation stack; X Y + intraday; B30-B100 blend weights. `candidates/README.md` lists
the sidecars of the named agents.
