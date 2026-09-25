# Progress

Goal: train a BC DayIntent policy (designs/day_intent.md) for the new day compiler
(copied from experiments/v10/sep24_day_compiler_opus, hashes in
data/COMPILER_SOURCE.sha256), validate the full agent on continuations and full
games, and fix the core issues, mainly in the compiler. Window 22:10-00:10 UTC.

## 22:10-22:25 setup, data, first model

- Data: 3,839 traces with team/submission metadata copied from
  experiments/v9/sep24_agent (2,224 top-20 perspectives). Fresh replay download
  (scripts/collect_replays.py) started in the background.
- Split: whole episodes (both seats) by SHA256 "sep24-BC-opus": 80/10/10
  train/validation/test. Test is unopened.
- Dataset = interface-coverage gate: 66,720 perspective-days, all represented,
  0 failed, 0 ignored, 0 fixed-value conflicts; 18,121 actions dropped by
  fixed-value rules as the design requires. Masks mark fixed values: no loss,
  never decoded.
- Native C++ inference matches PyTorch: 300/300 identical global decisions.

## 22:25-22:55 triage (user: "you must be missing something big")

- v1 (MLP, per-type argmax counts): 0/32 wins vs agent_sep23, continuations -4,047
  (10 days). Traces: new-crop counts decoded to 0 (argmax of a spread count
  distribution), and group fields were a per-member probability rounded to a
  count: this blurs whole-group decisions (experts act on whole groups 84-98%
  of the time). Both are output-design errors, not data or gradient bugs.
- Checks that passed: train vs validation errors equal (no overfit at width 128),
  network memorizes 512 dawns to 15.0 vs an exact entropy floor 14.3, native
  parity, per-field teacher-forced totals close to labels.
- Width 256 is better; it overfits after ~8k steps (best-validation checkpoint
  kept).
- Inputs v2: product workload calendars, capacity (free tiles, empty
  structures), affordability, demand, per-group price/value/deadline/shed
  distance; sum pooling; group heads conditioned on decoded whole-farm fields.
  v3: 10x10 tile CNN (own/opponent, shared weights). CNN alone did not lower
  validation loss.
- Output v5: 101-way count categoricals; v6: sequential count decoders as in
  the Sep 23 BC (each field sees the values already decided, size and legal
  maximum).
- The biggest issue was the compiler: when an early day could not be funded,
  the fallback dropped every new crop and animal (two idle days at game start).
  New fallback: trim new entities one unit at a time, most expensive first.
  Full games vs agent_sep23 (seeds 600-631, both seats):
  v2 14/64 -18,062 -> 42/64 +4,867 with the trim; v5 counts + trim 52/64 +12,984.

## 22:55-23:20 decoding design, fresh data, final gate

- User question: group fields were one per-member probability rounded to a
  count (an expected fraction, not a count distribution). Replaced by 101-way
  count categoricals. The Sep 23 BC already decoded counts sequentially with the
  assigned/remaining counts as inputs; v6 adopts that; v7 uses independent
  (marginal) count heads with exact MAP DP over the partition (user's idea).
- DP over sequentially trained heads failed (0/64, -133,765): those heads never
  saw counts above the remaining members, so their logits there are untrained.
  The teacher-forced decode check caught it (86% member mismatch).
- Fresh replays: 1,179 episodes (1,510 perspectives) downloaded and converted;
  coverage gate on 112,020 perspective-days: 112,017 represented, 3 ignored
  (discarded animal), 0 failures.
- Full games vs agent_sep23, seeds 600-631: v6 49/64 +10,297; v7 58/64 +15,711;
  v8 (sequential, fresh data) 54/64 +16,393; v9 (marginal DP, fresh data) 64/64
  +20,585. Selected v9. Fresh seeds 700-731: 61/64 +19,216; in-house league
  agents 62/64 and 64/64; test-split continuations -2,342 / -1,839 / -223.
- Continuation losses concentrate on heavy harvest days: workers carry 200+
  units into the night, the route solver cannot schedule the end-of-day returns,
  100+ units are destroyed. In our own full games discards are small (2.8 per
  game), because our farms are lighter than top players' day-20 farms.
- Wrote BC_issues_sep25.md in the repo root for outside advice.

## 23:20-01:20 opponents, cash safety, style conditioning, data

- Wider C++ panel exposed a total collapse vs teammate_shoprouter: day 0 ended at
  $0, no hires next day, no route, every crop and animal died. Fix: runtime cash
  guard (drop seed/animal buys that would leave < $8) and a next-dawn reserve that
  counts sellable assets. teammate_shoprouter 0/64 -> 64/64; cost vs agent_sep23
  (61 -> 48/64).
- Local-LB (official engine, Python agents, snapshot 3326cc6 = current origin/main):
  v9 barely beat the top 3 (30-37/64 wins). Team fine-tunes of v9: DECEM helped
  (40/64 each), DSM neutral, Mother-Goose worse.
- v11: team style one-hot (global slots 224+, 10% style dropout). Style 0
  (no team) is best. Pooled seeds 700-731 + 800-831 (128 games each):
  arsgorynich 94-34 +4,183 [2,253, 6,286], shiiin9 96-32 +4,480 [2,459, 6,574],
  ahmed 84-44 +4,678 [2,636, 6,875] (seed-clustered 95% CI). Selected v11.
- Data: official daily episode datasets 2026-07-30..09-22 imported; every episode
  from Aug 16 on replays exactly in our engine (about 25k episodes), Aug 14 and
  earlier all mismatch (rules changed Aug 15). Bulk API export of top-10 teams done
  (911 episodes). Meta Kaggle join (team, submission, dates, ratings) running.
- Ideas ledger: both ChatGPT reviews read in full; 28 ideas recorded.

## 01:20-02:00 compiler determinism, time, site capacity

- The compiler had two 2 s wall-clock cut-offs (return ladder, trims): results
  depended on machine load (2 of 8 games differed on a rerun). Replaced by fixed
  counts; repeat runs are now identical.
- Without the cut-off one day took 20 s (seed 801 day 25). New tools: the Local-LB
  bridge can record every observation (LB_DUMP=1) and build/lb_replay replays a
  game exactly, with DC10_DEBUG for one day.
- Causes: (1) the intent asked for 19 new crops with 18 free tiles; the solver
  failed at once and the trim loop reran the whole failing ladder 12 times;
  (2) at L2/L3 hour 0 had 9 hires + 1 fertilizer buy = all 10 order slots, so the
  sell that makes shed room had no slot (shed 94/100), and 6 cash-funding
  variants were tried for a failure that was not about cash.
- designs/day_intent.md requires valid intents to fit free tiles, and decoding to
  enforce it; our validate() and decoder did not. Fixed: free_sites() (open tiles,
  one-shot harvests and clears, one-shot crops that turn into weeds today, ongoing
  clears, land bought today) in validate(); the decoder re-decodes with fewer new
  entities (least log-probability loss first) until they fit. Coverage gate:
  112,017/112,020 represented, 0 failures (the first rule without weed-turning
  crops failed 268 expert days, found with tools/site_audit).
- The compiler digs weed-turning one-shot crops when their tiles are needed (same
  end state as digging the weed; the design says the compiler never digs other
  one-shot crops - flagged to the user).
- Trims retry only L0/L1; funding variants only after cash failures. Seed 801:
  compile time per game 28.4 s -> 7.5 s, worst day 20 s -> 1.7 s.
- Across 384 v11 Local-LB games before these fixes: 14% of days trimmed new
  entities (first failure: no route 1,074, next-dawn reserve 384, land purchase
  short 204, unit action fails ~370); 77 days fell to L2+ (39 to survival only).

## 02:00-02:50 compiler gate results, opponents, data sources

- Final compiler config (determinism, site capacity, weed digs, L0/L1-only trims,
  cash-only funding variants, fertilizer cap on shed-room failures), v11 style 0,
  Local-LB top 3, seeds 700-731 + 800-831 (128 games each):
  arsgorynich 108/128 (84%) +6,360 [4,499, 8,427]; shiiin9 108/128 +6,520
  [4,573, 8,514]; ahmed 98/128 (77%) +6,435 [4,477, 8,514]. Paired vs the old
  compiler: +2,177 / +2,040 / +1,757 per game, all CIs above zero. Trimmed days
  13.7% -> 4.9%; days at L2+ 77 -> 6. Worst compile overage per game ~6 s (budget 60 s).
- The Local-LB top 3 are one code family: arsgorynich vs shiiin9 differ in 243 of
  ~7,000 lines and give identical games in 56/64 (margin correlation 0.9999); ahmed
  differs in 512 lines (0.90). Added Local-LB #4-#6 (arlene x2, cha22) from 3326cc6.
- Meta Kaggle has no kaggriculture rows in Submissions/Teams (competition still
  running). Metadata now comes from EpisodeAgents (submission, ratings), Episodes
  (times), API listings (submission -> team, dates, scores) and the leaderboard csv.

## Audit 02:25 (24 h goal started ~02:05)

State: best agent = v11 style 0 + final compiler (84/84/77% vs Local-LB top 3, 128 games).
Done since last audit:
- Decision-push screening (seeds 700-715, arsgorynich + ahmed): every uniform push
  loses (new-animal total at the 0.35 quantile -6k, feed+care all -6k, crop quantiles
  0.35/0.65 -2k/-3k, land bias +-2 -1k/-2k; collect/harvest all ~0). Outcomes are very
  sensitive to the network's decisions, and the BC sits near a local optimum along
  simple axes; headroom must come from state-dependent choices (next: per-dawn search).
- Data: Meta Kaggle join (64,834 perspectives, 1,837 teams); ladder ratings drift
  (daily top-episode p90: 3,106 Aug 15 -> ~2,830 Aug 31 -> ~3,160 mid-Sep), so quality
  = relative strength (final rating minus the ladder p90 on the submission's last date).
  Corpus v5: 48,465 perspectives (final rating >= 2,700 plus v4); arrays v5: 1,453,768
  of 1,453,950 days represented, 0 failures; grids stored as float16 (22 GB).
- Training v12 (strength/recency conditioning, site features, 40k steps) and v12_aug
  (+ dihedral grid augmentation) in parallel; ~30 min each (data-loading bound).
- New evaluation: tools/replay_games (our agent vs frozen recorded top-team players,
  same seed); 145 held-out games of the current top-10 teams since Sep 18.
- Local-LB snapshot now has all 10 active agents (ranks 1-10 of 3326cc6).
- Root replays/ (593 GB raw JSON) being compressed losslessly with zstd (~245x).
Next (priority): evaluate v12 vs v11 (Local-LB top 3 + ranks 4-10 + replays); per-dawn
search headroom test; sales audit vs top players; expert iteration if search helps.

## Audit 03:45

- v12_cond / v12_aug trained (40k steps, ~46 min each; validation 17.23 / 17.30 on the
  v5 split; augmentation slightly worse on unaugmented validation). Panels running.
- Per-dawn search (tools/search_games; days 0-12; 7 day-level pushes; full-game
  rollouts with exact copies of both agents; seeds 900-907 both seats):
  king_rc4 16/16 +11.9k -> 16/16 +37.2k; ahmed_v25 14/16 +12.9k -> 16/16 +50.7k.
  Search changes ~4 of 13 days per game, mostly the new-crop total quantile (day 0 in
  half the games). With an exact opponent copy the search may be steering a chaotic
  opponent; mismatch test running (rollouts vs king_rc4 / ahmed_v25 / our v11 as the
  opponent model while playing another opponent).
- Local-LB behaviour groups found; panel uses one agent per group (6) x 64 games, plus
  8 extra Python agents (public-notebook candidates + teammates' PPO/RL submissions;
  the third download was byte-identical to Local-LB #1), 7 C++ agents, 145 replays.
- New tools: trace writer (save_replay; written traces pass trace_check), trace output
  in full_games/search_games, bc:<model> opponents (league of our versions), zoo
  fine-tunes of v12_cond on the Kaggle top-6 teams (running).
- Resolved: the compiler digs one-shot crops that turn into weeds today when their tiles
  are needed (user: proceed without asking).

## Audit 04:20

Results:
- v12_cond (no style slot) vs v11, full panel 865 games: wins 87% -> 92%, margin
  +20.2k -> +22.3k, paired +2.1k [+5, +4.5k]; Local-LB 79% -> 88%. New best model.
- Style: v12 with style 0 is much worse (Local-LB 88% -> 53%, all -6.6k): in the v5
  corpus index 0 = every team without its own index (most of the data) plus dropout, so
  "style 0" = the average unknown team. The all-zero style input works. train.py gets
  --style-v2 (unknown teams -> 31, dropout -> no style) for the next models.
- Strength input steers play: -100 vs +150: Local-LB -2.5k [-5.1k, -0.2k], C++ +4.2k;
  +300: Local-LB -2.8k (partial). Keep +150.
- Compiler on v11: no next-dawn reserve: Local-LB 84% -> 95%, all 91% -> 98%, +3.0k
  [-1.2k, +7.5k], no collapse vs teammate_shoprouter (32/32; the runtime cash guard
  covers it). Crops-first trims: -3.4k [-6.0k, -0.5k] (rejected).
- Deviation analysis: network unbiased on expert states; closed-loop gaps start in the
  opening (unspent cash days 1-2 caused by the reserve; land a day early; fewer day-6
  animals). Top players end day 0 with ~$7 and skip some feeds (escape needs 2 days).
- Training length: 10k steps valid 20.05 vs 40k 17.23 (full games pending).
Running: v12 + no reserve (confirm), whole-game and opening style pins (6 top teams),
data-lever screens (all/top/recent/winners 10k), zoo, search mismatch, v12_aug panel.

## Audit 04:50

- Training length is a large lever: d_all10k (10k steps, valid 20.05) vs v12_cond (40k,
  17.23): Local-LB 57% vs 88% wins, all -4.8k [-8.6k, -1.0k]. Started v13_style2 (60k,
  style v2) and v13_w384 (width 384, 120k).
- On v12, no reserve is worse (-3.0k; seed 706 collapses: day 6 spends all cash on
  cows + land, animals go unfed on days 8-10 and escape, $62k vs $114k). The reserve is a
  needed mid-game safety net; testing a reserve that starts at day 1 (day 0 only free).
- No cash guard: -0.8k (kept). Crops-first trims: -3.4k (rejected).
- Whole-game DSM style on v12: neutral (+0.5k). DSM style for days 0-5 only, pooled
  after: Local-LB 81% -> 97%, all 91% -> 98%, +5.6k [-0.8k, +12.7k] (mini screen).
  Other teams' openings running; the best goes to the full panel.
- League (C++): v12 vs zoo_dsm 24/32 +2.1k; vs zoo_decem 20/32 +3.3k.

## Audit 05:15

- Target metric: the ladder counts only wins, so decisions weigh win rate first;
  panel_compare now reports paired win-rate change with a seed-clustered CI.
- DSM opening (days 0-5), full Local-LB panel: wins 88% -> 93% (+4.7 pts [-4.7, +14.1]),
  margin flat (+63). Mini-screen margin gain did not hold at scale.
- Losses cluster by seed (both seats, all opponents): v12 loses seed 700 10/12; the DSM
  opening fixes seed 700 but loses seed 719 12/12. Seed-level outcomes are chaotic
  (719: games equal for 11 days, then +12.4k vs -2.0k).
- Opening pins (mini screens, all 128 games): DSM +5.6k, Mother-Goose +2.3k, DECEM +0.3k,
  M&M -3.2k; whole-game pins all <= +0.5k.
- Zoo on screen vs v12: Majkel fine-tune +2.4k, wins 94% -> 96%; Vadim +3.0k, wins 93%.
  League head to head: Majkel and Vadim fine-tunes beat v12 (13/32, 14/32 for v12).
- Data filters at 10k steps: recent-only -2.6k; top-strength-only -11.3k (both vs all
  data at 10k). More data wins.
- Width 384 learns faster: valid 17.63 at 16k steps (v12: 18.45 at 16k, 17.23 final).
- Search transfer test restarted at normal priority on days 0-6 (the first run was
  starved at nice 10).

## Audit 05:25

- Opening pins on mini screens (win-rate change vs v12 91%): Vadim +9.4 pts (100%), DSM
  +7.8 (98%), Majkel +3.9, Mother-Goose +3.1, DECEM -1.6, M&M ~-25. Whole-game pins are
  worse (Majkel -14 pts). DSM opening on the full panel: Local-LB +4.7 pts (88% -> 93%).
  Vadim opening full panel running.
- Majkel fine-tune + DSM opening: 96% wins (= Majkel alone), +3.5k vs v12.
- Data matrix done (10k steps, vs all data): winners-only -4.3k / -16 pts, recent-only
  -2.6k, top-strength-only -11.3k. All filters lose: the data lever is quantity and
  training length, not selection.
- New: whole-farm head ensemble (BC_ENSEMBLE; build_snap2); screening v12 + Majkel +
  Vadim. Large C++ panels (1,024 games per candidate, seeds 1000-1127) for v12, DSM /
  Vadim openings, Majkel, Majkel + DSM opening, for statistical power.
- Training: v13_style2 ~50%, v13_w384 ~25% (valid ahead of v12 at equal steps).

## 05:30-09:25 session lost (out of memory); audit 09:35

- Three concurrent training runs (+ a fourth loading) and ~26 game workers exhausted RAM;
  the session died and all jobs stopped. New rule: one training process at a time.
- Survived: v13_w384 to step 29.5k (valid 17.01, already below v12's 17.23), v13_style2
  to 33k (17.34), DSM-opening panel (Local-LB part), 329/384 Vadim-opening Local-LB
  games, search transfer results. Lost: v14 (all data), big C++ panels, rest of panels.
- Search transfer (ahmed_v25, 16 games): exact opponent copy days 0-12 +37.8k; with a
  wrong opponent model (king_rc4) +9.4k, wins 88% -> 100%; exact days 0-6 (8 games)
  +33.0k. About a quarter of the headroom survives a mismatched opponent model: large
  room for RL / expert iteration in the network decisions.
- Test-time search is not affordable on Kaggle (dozens of full rollouts per dawn vs a
  60 s overage budget) -> expert iteration offline: scripts/expert_iter.sh (v12 searches
  days 0-6 vs 6 opponent pairs with a different rollout model; 192 games, traces).
- Resumed: v13_w384b (from step 29.5k, 60k more steps, lr 5e-4, single process);
  screening the 29.5k checkpoint.

## Audit 10:10

- Vadim opening full Local-LB panel: 88% -> 94% (+5.7 pts [-0.5, +13.5]); vs ahmed and
  vs arsgorynich +12.5 pts [+3.1, +25.0] each. DSM opening 93%.
- Opening length (Vadim style) vs 6 days: 3 days -4.7 pts, 9 days -7.8 [-15.6, -1.6],
  12 days -9.4. Keep 6 days.
- The better openings plant more wheat on day 0 (9 vs 8), fewer melons (10-11 vs 13 over
  days 0-2) and more strawberries earlier (10.3-10.8 vs 8.7 over days 2-4).
- Headline candidate packaged: models/cand_v12_vadim6 (v12_cond + model.bin.opening
  "6 8"); the sidecar reproduces the env-driven games exactly. Full panel completing.
- Expert-iteration traces extract cleanly (600/600 days, 0 dropped actions).
- v13_w384b (resumed wide model): valid 16.53 at 7k steps (v12 17.23).

## Audit 11:55 (session resumed 11:05)

Compile time (recorded 8 games, scripts/opt_check.sh now compares every emitted action by hash):
- Exact changes (8/8 identical actions): quiet pair skip, threshold scoring with early abort,
  order-independent pickup floor in the route bounds. Net speed ~0 (timing noise +-5%): refine
  candidates are rejected by <1,000 points, mostly on routes already carrying lateness, so
  time/pickup bounds cannot prune them. Memoising reorder_jobs: only 2% of calls repeat.
- Where the time goes: failed market-return re-solves are 23 of 80 ladder solves but 81% of
  ladder time (~311 ms each vs ~22 ms for a success). Cause (formulation, not search speed): the
  market target asks all carried units of a product by hour 20, including units harvested after
  hour 19; the solver then scans every hire level x ~40-70 route heuristics before failing, and the
  day falls back to near-zero returns. A successful market re-solve may also add a $144-233 hire
  to sell units one day earlier (holding costs ~5%).
- Reformulation (knobs): DC10_FEASIBILITY_FIRST (portfolio at the max affordable workforce first,
  then hires downward with the winning config; infeasible day = one level), DC10_MARKET_TIMING
  (market target <= units the base schedule carries at an hour from which the shed is reachable
  by the deadline), DC10_MARKET_HIRE_VALUE (extra hires only while the sale gain pays their
  wages). Recorded games: compile -64% (worst day -75%), wages -$609/game, market-return days
  124 -> 136. Full games: running (partial -2.2k [-8.1k, +1.5k], -4.2 pts: not significant).
- Determinism: the current build reproduces the recorded baseline panel exactly (12/12 games).

Models: v14_all (v13_w384b fine-tuned on v5 + v6x, 40k) with the Vadim opening: Local-LB 88% vs
94% for v12 + Vadim (-5.7 pts [-15.9, +3.6]); not better. v15_w512 (cold start, all data,
augmentation, 120k steps) training (~9 steps/s, data-loading bound).

Weak points ranked from the best panel's 11,520 dawns: trims on day 0 (all 384 games; the
intent is ~$50 over budget and the trim removes a $500 sheep, leaving ~$470 unspent) and day 6
(360/384); land dropped on day 7/9 (~50% of games). A crops-first trim lost 3.4k on v11, so
leftover cash may be protective; next idea there is a shortfall-sized trim, only with games.

Local-LB origin/main moved to febb5a8: new #2 wzhengbiao-v15stack (1751.8); snapshot
data/localLB_febb5a8 (LB_SNAPSHOT env in lb_play.py); baseline games vs it running.

## Audit 12:15

- Local-LB PR: branch submit/pavel-bc-opus-v12-vadim (T3pp31/kaggriculture-localLB) with the
  v12 + Vadim-opening agent and the feasibility-first compiler; validator OK, 2/2 wins vs
  wzhengbiao-v15stack via lb.match; user opens the PR from the pull/new link (no gh CLI here).
- Compiler A/B (paired vs v12_open6_vadim, 384 Local-LB games): first feasibility-first version
  -152 [-1.2k, +1.0k], wins -1.0 pts; vs wzhengbiao 60/64 vs 62/64. Cause: the descent kept the
  config that won at the max workforce. Final version (descend to the scan's first level, canonical
  re-search, old narrow minimization) is decision-identical on 240 recorded days and 16 games
  (3 more games diverged first in the nondeterministic yannik-suffix opponent's state).
- Timing-capped market target alone (with FF v1): -306 [-2.0k, +1.4k], wins -3.5 pts: dropped;
  market-timing and hire-value code removed.
- Baseline vs the new Local-LB #2 wzhengbiao-v15stack: 62/64, +9.4k.
- Running: DC10_TRIM_MODEL arm (14 procs), DC10_LAND_FIRST arm (6 procs); v15_w512 at 31k/120k.

## Audit 12:25 (24 h goal restarted 12:15 Sep 25)

Done since 12:15:
- Evaluation fix: seeds 700-731 selected the Vadim opening, so changes to days 0-9 regress there.
  Fresh seeds 1200-1231: baseline (v12 + Vadim, FF compiler) 88% wins / +8.4k (255 games so far)
  vs 94% / +10.9k on 700-731. All A/B now on fresh seeds.
- Rejected on fresh seeds: decoder likelihood-per-dollar trim order (-1.1k [-2.6k, +0.2k]);
  land-first trimming (identical plans: the baseline already keeps land via level-0 trims).
- Research (Lux AI S3, Orbit Wars 2026, Kore 2022): BC alone reached top 10, BC + RL fine-tuning
  top 5, pure large-scale self-play RL won Orbit Wars (200M params, 15B steps); conditional BC
  (teacher one-hot + recency) and TTA/ensembles were standard; Kore's 1st place decoded actions
  autoregressively (relevant to our independent count decoding).
- Data: public replay DB (xishengfeng) with 54k new episodes Sep 7-25 (39k with a >= 2700 player,
  i.e. more strong recent data than our whole current corpus period); converter verified exact
  (actions + 720 parity hashes vs official traces). Import running.
- Flexibility test running (reports/flex): 8 network pushes vs 8 compiler variants on 48 fresh
  games vs arsgorynich / ahmed / wzhengbiao; per-game best-of flips = headroom per part.

Priorities: (1) DB data -> arrays -> v16 (more data was the biggest lever so far); (2) flexibility
result decides whether network-side search/expert iteration (RL precursor) comes next; (3) v15_w512
screen when training ends (~15:30).

## 13:00 PR agent on the Local-LB: 333/400 (83%), Elo 1891.7 - why lower than local

- Time limits ruled out: through the Local-LB runner (real 1 s + 60 s overage accounting) under
  load ~60 the packaged agent used <= 0.7 s of overage per game; the valve needs < 25 s left.
- Our lb_play reproduces the Local-LB runner game for game (10/10 margins), and replaying the
  Local-LB's own seeds (opponent i alphabetical: seeds 1000*i .. +19, both seats) reproduces its
  W-L exactly for every opponent checked (7/7 so far).
- So the host did not hurt us; the local estimate was optimistic: seeds 700-731 selected the
  opening (fresh seeds 88%, not 94%), the panel omitted haideptry (70%) and sunil (75%) as
  "identical to arlene-idle" (they are not), and mirrored seats halve the effective sample
  (40 games ~ 20 outcomes, +-10 pts).
- Fix = strength vs the weak matchups (ahmed 70%, haideptry 70%, sunil 75%, cha22 75%): panels now
  include haideptry and sunil; the losing Local-LB games are the analysis set (reports/lbseeds/base).
- Data: replay DB imported: 41,423 verified episodes; corpus_v7db 57,607 perspectives (>= 2700
  submissions), 1.73M days, coverage gate 1,727,837 represented, 0 failures. v16_db training
  (v12 recipe, v5 + v7db, 80k steps); v15_w512 stopped at 48k.
- Update: all 10 opponents reproduce the Local-LB W-L exactly (333/400 = 83%).
- Evaluation protocol from here: select on the broad unseen panel (all 10 Local-LB agents, seeds
  1300-1323, both seats, 480 games, scripts/panel_lb10.sh; dawns now record shops and prices),
  confirm the final candidate on a holdout (seeds 1500-1523) before packaging.

## Audit 12:50

- Local-LB PR result fully explained (exact replay of all 400 games). Standard now: broad unseen
  panel scripts/panel_lb10.sh (10 Local-LB agents, seeds 1300-1323; dawns record shops/prices).
- Flexibility (48 fresh games vs the 3 strongest): whole-game best-of flips all losses for network
  pushes and compiler variants alike -> losses are fragile; single settings: crops-first trims
  +4.4k, crop quantile 0.35 +3.4k, DSM opening +1.8k, no reserve -22.2k.
- work/sep25_bc_weakness (other session) REPORT.md: herd too small (day-0 sheep trimmed in 100%
  of games; reserve ignores overnight fertilizer), melon sale races lost (-$42/melon), wages
  +$1.7k; X1 +5.1k, X4 +6.8k on seeds 700-715. Ported as DC10_TRIM_D0_CROPS, DC10_RACE,
  DC10_RACE_DEADLINE, DC10_SALE_TIE_NOW (defaults exact: 8/8 recorded games).
- Seed dependence (lb10 base, 219 games): all 37 losses are games with an egg shop (BAKERY or
  BRUNCH_SPOT) unlocked by day 12 (loss rate 21%); 0 losses in 68 games without one. In those
  losses we hold 2-3 sheep vs the rival's 4-6 and 5.9 vs 8.0 cows at day 9.
- Running: lb10 base (480), X1 and X4 (seeds 1300-1311, 240 each); v16_db training (CPU-starved,
  ~9 steps/s).
Next: X1/X4 vs base on the egg-shop losses; if X4 wins broadly -> holdout 1500-1523 -> new branch.

## Audit 12:52 (second)

- Broad unseen panel baseline (PR agent): 413/480 (86.0%), +9.9k.
- X1 (day-0 crops-first trims + next-dawn reserve from day 1), selection seeds 1300-1311:
  +7.4k [+2.3k, +13.7k], wins 85% -> 100% (236 games); all 20 egg-shop losses turned into wins.
  Holdout 1312-1323 (partial, 98 games): +5.8k [-2.6k, +14.0k], wins 86% -> 94%.
- Rejected vs X1 (paired): farm-aware reserve on all days -1.3k [-6.1k, +1.8k]; melon race (X4)
  -1.5k [-5.1k, +1.5k]; no opening pin -3.1k [-7.6k, +0.7k]. DSM opening +1.0k [-3.0k, +4.6k]
  (running to 240 games).
- X1 is now the compiler default (DC10_TRIM_D0_ANIMALS=1 / DC10_RESERVE_FROM_DAY=0 restore the
  old rules; old path exact 8/8; new default = tested X1 arm 8/8).
- Running: holdout confirmation, DSM-opening arm, per-dawn search space comparison (network pushes
  vs compiler variants, exact copies of ahmed_v25/king_rc4), v16_db training.
Next: finish holdout; package v12_vadim6 + X1 as the new branch (after the DSM and v16 checks).
- 13:05 New branch submit/pavel-bc-opus-v12-herd (v12 + Vadim opening + X1 compiler defaults),
  package submissions/sep25-bc-opus-v12-herd (portable build = X1 arm 8/8 games), validator OK.
  Predicting its Local-LB result by replaying the Local-LB's own 400 seeds (reports/lbseeds/herd).
- X1 holdout losses: one seed (1323) x 4 near-identical opponents; our own money was higher than
  the baseline's there (+$14k); the rival's play diverged (chaos), not a herd problem.
- Holdout final (seeds 1312-1323, 240 games): X1 +5.8k [-1.2k, +12.1k], wins 87% -> 95%, positive
  vs all 10 opponents (+3.7k..+8.2k). 480 unseen games: ~+6.6k, wins ~86% -> ~97.5%.
- Under X1: DSM opening vs Vadim +0.5k [-3.0k, +4.3k] (keep Vadim); crops-first trims on every
  day -12.8k [-32.4k, +0.8k] (61 games; stopped): the day-0 rule only.
- Local-LB seed replay of the herd agent (the Local-LB's own 400 games): 398/400 vs 333/400 for
  the PR 1 agent; every opponent 40-0 except shiiin9 38-2 (unchanged); ahmed +7.6k, sunil +9.3k.
- Gates with X1: extra Python agents (112 paired games) 99% vs 97% wins, +2.0k [-2.9k, +7.7k]
  (teammate PPO agent 81% -> 94% for us). The first C++/zoo gate run used a stale full_games
  (partial --target builds): discarded; rerunning from build_gates (all targets rebuilt; checked:
  old-rule knobs reproduce the recorded baseline game exactly). Build all targets before copying.
- Search headroom (X1 agent, exact copy of ahmed_v25, days 0-12, 16 games): no search +18.4k,
  per-dawn network-push search +32.8k (+14.4k). Compiler-variant search running.

## Audit 13:22

- Deliverable: branch submit/pavel-bc-opus-v12-herd, predicted 398/400 on the Local-LB's own games
  (PR 1 agent 333/400). Gates so far: extra agents pass; C++/replay/zoo rerunning.
- The seed-dependent weakness (egg-shop seeds) was the day-0 herd funding; fixed by X1.
- Evidence for RL (user question): per-dawn network-push search +14.4k/game vs exact ahmed_v25
  copies (compiler-variant control pending); whole-game best-of was uninformative (both parts flip
  all fragile losses).
- Priorities: (1) v16_db eval with X1 (training ends ~14:00); (2) compiler-vs-network search
  control; (3) expert iteration with X1; (4) sale timing (graded receipts) and wages.
- Search headroom, X1 agent vs exact ahmed_v25 copies, days 0-12, 16 games: no search +18.4k;
  per-dawn network pushes (7) +32.8k (+14.4k); per-dawn compiler variants (7) +27.2k (+8.8k).
  Network decisions have more state-dependent headroom than the compiler options tried, but both
  are large (oracle opponent). king_rc4 runs in progress.
- Old gates with X1 (vs v12_open6_vadim panel, paired): C++ 224 games 100% wins, +5.3k [+2.5k,
  +8.3k]; replays 90% vs 88%, +0.6k; extra +2.0k; all pooled +3.1k [+0.6k, +5.5k]. Pass.
- Zoo league (network gate, both sides X1): v12_herd vs top-team clones 20-27/32 wins each
  (DECEM 23, DSM 22, Goose 22, Majkel 27, M&M 20, Vadim 24), ~72%.

## 13:55 Robustness fix: X1 collapses on replays -> "trim0"

- Replay gate (145 frozen top-10 games): X1 has 3 farm collapses (own $6-21k, margins down to
  -119k); the baseline had none. Isolation on those 3 games: the collapse needs both halves of X1;
  crops-first day-0 trims with the day-0 reserve kept (trim0) win them (+44k, +10k, +35k); the
  farm-aware reserve avoids the collapse but loses margin.
- trim0 = day-0 crops-first trims + next-dawn reserve from day 0 (now the default;
  DC10_RESERVE_FROM_DAY=1 gives X1). Broad panel 1300-1311: +6.6k [+3.1k, +9.9k] vs base, wins
  85% -> 100% (240); vs X1 -0.6k [-5.9k, +3.5k]. Replay gate: 0 collapses, 91.0% wins vs 87.6%
  (base) and 89.7% (X1), +4.5k vs base.
- User: keep branch submit/pavel-bc-opus-v12-herd (X1) as is; the fix goes to a new branch
  (submit/pavel-bc-opus-v12-robust; package submissions/sep25-bc-opus-v12-robust = trim0, 8/8 games).
  Running: its Local-LB-seed replay, C++ and extra gates; v16 zoo league.
- v16_db (v12 recipe + replay-DB data, 80k steps) zoo league (both sides X1): 91/192 (47%) vs v12's
  138/192 (72%); worse vs 5 of 6 top-team clones. Broad-panel sample running (reports/lb10/v16_trim0).
- v16 + trim0 vs v12 + trim0, broad panel 1300-1305 (112 games): -8.9k [-12.3k, -5.6k], wins 100% ->
  91%. The replay-DB data (median strength -187 vs v5 -116; 3% >= 0 vs 9%) makes the model worse.
  v17 (same data, longer) stopped. Next data try: only the stronger DB part, after checking that
  DB strengths are on the v5 scale.

## Audit 14:15

- Local-LB after the herd merge: pavel-bc-opus-v12-herd #1, Elo 2283.1 (394-6); v12-vadim #2
  1800.3; new ttyn-kaggriculture-master-engine-v3 #3 1755.8. Active set now includes our two agents
  and ttyn (haideptry, sunil, yannik2 dropped).
- Branch submit/pavel-bc-opus-v12-robust pushed (trim0 compiler; main.py sets the model path per
  game). Gates vs v12-vadim: extra +3.9k, C++ (6/7) +4.6k, replays +4.5k with 0 collapses; LB-seed
  replay (old set) 398/400. Replay of its new evaluation set (reports/lbseeds2/robust) running.
- Data: v16 (all DB) -8.9k: the DB's weaker bulk dilutes; strengths are on the v5 scale (same
  teams within 10-30 points). v18 = v5 + DB perspectives with strength >= -116 (11,410
  perspectives, 342k days, coverage 0 failures), 50k steps, training.
- Expert iteration data restarted with the robust compiler (reports/expert_iter_robust).
Next: v18 eval (zoo league + broad panel); expert-iteration fine-tune; compiler: sale timing.
- Robust agent (trim0) old gates vs v12-vadim, 481 games: C++ +6.1k [+1.8k, +10.4k] (100% wins),
  extra +3.7k, replays +4.5k (0 collapses); pooled +5.0k [+1.4k, +8.5k].
- Melon race (tools/melon_race, 8 traced games vs king_rc4): robust sells day-10 melons at hour 20
  ($231; rival 60 units at hour 9, $254) and the rest on days 11-12 ($194, $158). Race knobs with
  receipt deadline 9/10: we sell first at hour 9 ($271/$263), margin +2.1k/+3.0k; deadline 12 (as
  in X4) -1.0k. Broad panel for deadline 10 running (reports/lb10/race10).

## Audit 14:25

- Robust agent, replay of its exact Local-LB evaluation (active set of 7a824c1): 367/400; 39-40 of
  40 vs every third-party agent (ttyn master engine v3 40-0), herd 19-21, vadim 30-10. Next agent
  must beat herd head-to-head.
- Race deadline 10 (DC10_RACE + DC10_RACE_DEADLINE=10 + DC10_SALE_TIE_NOW) on robust: broad panel
  +1.1k [-0.4k, +2.4k]; replays 139/145 vs 132/145 (+4.8 pts [+0.7, +9.6]); C++ +0.6k; gates pooled
  wins +1.9 pts [+0.3, +4.0]. Keep.
- work/sep25_bc_weakness X10: budget trims ordered by the network's log-probability loss (not per
  dollar) +5.3k Local-LB, +6.2k and +25 win points vs the BC zoo (all over-budget days). Our
  DC10_TRIM_MODEL now uses loss-only order. Probably run with the day-0 reserve off -> testing with
  the reserve kept (collapse check) and X10-exact on the replay gate; candidate tmrace (trim model +
  race 10) on the broad panel.
- Other session's proposals recorded: value model V(s) (VALUE_MODEL.md), night-inventory planning
  (OPPORTUNITY_night_inventory.md), boundary analysis (BOUNDARY.md).

## Audit 14:50 - ranked ideas, budget revision, recovery level

Ranked (conceptual first; IDEAS_LEDGER): 1 budget revision (network chooses what to give up),
2 production-driven opponent forecast (generalises the melon race), 3 value model V(s),
4 expert iteration, 5 compiler horizon across the night, 6 strong-DB data (v18), 7 RL.

- Budget revision as a covering knapsack over units with the decoder's marginal log-prob losses
  (DC10_BUDGET_REVISION, revise_for_budget): works mechanically, but the "dollar shortfall" is the
  wrong abstraction: day 6 fails on hour-0 cash timing (land + hires + wheat at hour 0 vs $1,071),
  which removing entities does not fix unless hires drop. Kept as an option; the verified
  per-unit form (network order, compile after each removal) is the default now.
- Candidate 1 (network-ordered trims + melon race, day-0 reserve kept) = new defaults
  (DC10_TRIM_PRICE / DC10_NO_RACE / DC10_SALE_TIE_LATER restore robust; both directions exact 8/8):
  replays 137/145, +4.8k [+0.1k, +9.5k] vs trim0; broad panel +1.4k; BUT head-to-head vs herd 6/22
  (-3.8k): with the day-0 reserve it trims 3 wheat + 3 melons where herd trims 1 melon.
- Collapse mechanism (herd rules, replay 112850054): not day 0 but a day 7-8 cash crunch; the
  ladder then falls to survival-only, which disabled every harvest and collection and capped feed
  by morning cash: $15-20 for six days, all 12 animals lost, 38 plants decayed.
- Recovery level (DC10_RECOVERY): survival keeps the network's harvests and collections and feeds
  from cash plus 80% of that income. On the 3 collapse games with herd rules: -118.8k/-78.7k/-95.4k
  -> +2.8k/+1.9k/+20.9k. Candidates now: c2a = herd rules + recovery; c2b = network trims + no
  day-0 reserve + recovery + race. Replay gates and broad panel (c2b) running.

## Audit 15:25 - test harness bug, v18, value model, sell order

- Harness bug (fixed in scripts/lb_play.py): pavel-bc-opus-v12-herd and -vadim set BC_OPUS_MODEL at
  import, and our bridge reads it in opus_new, so in their games our challenger loaded THEIR model
  folder (same v12 weights, no .compiler sidecar = default compiler). Every sidecar h2h vs herd /
  vadim / robust so far is invalid (c2c "13W-14D-13L vs herd" was defaults vs herd), and env-knob
  h2h leaked the knobs into herd's .so. lb_play now reads our model path before importing opponents
  and sets it only around our opus_new. Panels without our own agents (panel_lb10) were unaffected.
- v18_dbstrong (replay DB, strength filter): Local-LB panel 1300-1305 120/120 like v12 (-0.6k
  [-7.7k, +7.3k]); zoo league 97/192 vs v12's 138/192. Rejected; the DB data does not help (v16, v18).
- Value model value_v1 (replay dawns, final margin): held-out R^2 0.36-0.58 on days 3-9 (ridge on
  the same features 0.01-0.15), 0.80 by day 18, win accuracy 0.67 on day 5. Next: decision test
  on exact-copy rollouts (tools/search_games SEARCH_ROLLOUTS, scripts/value_decision_test.py):
  does argmax V over the 7 decision pushes beat the baseline candidate?
- Sell order (CompileOptions sell_by_price, sidecar key): the engine fills both players' orders
  slot by slot, so a sale listed earlier than the opponent's sale of the same product takes the
  higher prices. merge_orders listed sales wheat -> fertilizer (herd too); new option lists them
  by descending price. Defaults unchanged (opt_check 8/8). h2h vs herd/robust rerunning with the
  fixed harness (cand_sp, cand_c2a, cand_c2csp).

## Audit 15:40 - slot priority confirmed head to head

- Fixed harness, seeds 1300-1319 both seats (40 games each), paired against the same games:
  | challenger | vs herd | vs robust |
  |---|---|---|
  | c2a (herd rules + recovery, old sale order) | 13/40, +26 (pure mirror, seat-symmetric) | 31/40, +3.5k |
  | sp = c2a + sales by descending price | 36/40, +9.2k | 40/40, +11.4k |
  | c2csp = sp + melon race + tie-now | 33/40, +14.5k | 37/40, +7.5k |
  sp is the lead; the race adds margin vs herd but loses wins. work/sep25_bc_weakness found the
  same lever independently (X13: sales by revenue at stake; zoo +8.9k, 222/224; Local-LB +0.1k).
- sell_by_price generalised to CompileOptions::sell_order (0 product index, 1 price, 2 revenue at
  stake; sidecar key sell_order, env DC10_SELL_ORDER); defaults exact (opt_check 8/8). C++ mirror
  so2 vs so1 (stake vs price) and each vs c2a running (tools now give BC rivals their model's own
  opening; before, rivals played without the opening sidecar).
- value_v2 (TD(0.8) targets from value_v1, GPU-resident data: 30k steps in 4 min instead of ~35):
  same R^2 as v1 (+0.03 on days 4-5). Decision test pending (king_rc4 rollouts 816/840).
- Local-LB main is now d603858 (robust merged; active set: herd, robust, vadim, ttyn, wzhengbiao,
  arsgorynich, shiiin9, ahmed, arlene-wheat, cha22). scripts/lb_seeds3.sh replays a new
  challenger's exact Local-LB games for that set.

## Audit 15:50 - sale order key, value decision test

- Sale order, C++ mirror games (build_so, seeds 1300-1319 both seats, rivals with their own opening):
  stake order (so2) vs price order (so1) 36/40, +3.2k; so2 vs old order (c2a) 39/40, +11.6k; so1 vs
  c2a 36/40, +9.2k (identical to the Python h2h so1-like sp vs herd: the harnesses agree).
  -> sell_order 2 (revenue at stake) is the candidate. Broad Local-LB panel (price order, 240
  games): +166 [+30, +329] vs herd rules, 100% both: no cost vs third-party agents.
- Value decision test (king_rc4, exact-copy rollouts of the 7 pushes, days 0-9, 32 decision
  dawns after removing mirrored seats): argmax V regret $3.4k (v1) / $3.8k (v2 TD) vs baseline
  candidate $1.4k and random $2.5k; top-1 0.38 (random 0.14). The replay-trained V is informative
  on held-out replays but does not rank our own alternatives: not usable for decisions. Next
  step only with on-policy paired data (rollouts are saved as traces for that).
- Expert-iteration data stopped after 5 pairs (239 changed days of 896 searched): too small to
  move the network; CPU goes to gating so2 (lbseeds3 exact Local-LB replay, gates).

## 16:00 - branch 3 pushed: submit/pavel-bc-opus-v12-slots

- Package submissions/sep25-bc-opus-v12-slots (models/cand_so2: v12 + Vadim opening, sidecar
  "sell_order 2"; herd rules + recovery by default). Validator OK; Local-LB runner check vs herd
  identical to the dev build on 6 games.
- Exact Local-LB games of the d603858 active set (scripts/lb_seeds3.sh): 356/357 before the push
  (herd 40/40 +10.9k, robust 39/40 +10.3k, vadim 40/40 +10.6k, third-party agents 40/40; ttyn
  and wzhengbiao finishing).
- C++ mirror add-ons on so2: melon race 20-20 (+0.5k), network-ordered trims 16-14-10 (+0.3k):
  neutral, not included.
- JJ's PPO agent (ppo-v6n-it60, branch add-ppo-v6n-it60; Majkel BC + 60 PPO iterations vs a league
  of 15 public / older Local-LB agents, none of ours): so2 11/13 wins; both losses on seed 1300
  (-0.8k, -1.1k): equal farms, we led by $1.7k cash at the last dawn; PPO kept younger strawberry
  plants to the end and sold 39 strawberries at hour 21 of day 29 after the price recovered from
  $57 to $105; we cleared old strawberries on day 26 and replanted wheat/carrots. Wider PPO panel
  (seeds 1310-1349, hourly logs) running.

## Audit 16:10 - PPO weaknesses, wage-aware returns, seed lottery

- so2 final: exact Local-LB replay 399/400 (+13.1k/game); gates: extra 111/112, C++ 224/224,
  frozen replays 135/145, zoo 190/192 (v12-herd 138/192). Pushed as submit/pavel-bc-opus-v12-slots.
- PPO (ppo-v6n-it60), 68 games so far: 65 wins; 3 losses, all ~-1k (seeds 1300 x2, 1316).
  Ledger (scripts/ppo_ledger.py over LB_ACTIONS hourly logs), ours minus PPO per game: wages
  -1.6k (20/21 games), fertilizer sold -1.5k and bought -0.6k, melon -0.7k; we win on
  strawberries +7.5k, tomatoes, wool, eggs, carrots. PPO holds 11 hires every day ($232); we use
  12-13 hires on 39% of days 10-29 ($376-609): mean $316 vs $223 per day.
- Debug logs: most 12-13-hire days are same-day market returns on top of a 10-11-hire base
  (11->12, 11->13, 10->13, 12->13); the ladder accepted them whenever the sale DP preferred
  selling tonight, ignoring the marginal Fibonacci wage.
- return_wages (sidecar key): keep the market part only if its sale-DP gain covers the extra
  wages. Mirror vs so2 28-10-2 (+646); Local-LB panel 1300-1305 vs sp +1,838 [+362, +4,041], 9/10
  opponents positive (the other session's X5 lost -1.8k: it compared per-product gains with all
  wages outside the ladder). Exact Local-LB replay (lbseeds3/so2rw) running.
- fert_value (fertilize only if the extra yield at today's price covers the fertilizer's price):
  mirror 21-13-6, -45: neutral, off.
- Care: our feed/care per animal 0.98/0.86/0.76 on days 10-14/15-19/20-24; top players (arrays_v5,
  strength >= 100) 0.93/0.87/0.83: the network imitates them. Rules: every care+feed day banks
  +1 product. Probe full_care (feed+care every existing animal while care banks) running.
- Seed lottery: v12 recipe with seed 1 (valid 17.205 vs v12 17.229) loses to v12 8-32 (-2.8k)
  under the same compiler. Game strength varies strongly across seeds at equal validation loss;
  seed 2 training (GPU); select networks by games, not by validation loss.

## 16:20 - slots is Local-LB #1; wages candidate; network screen; CPU policy

- Local-LB (df4172a): pavel-bc-opus-v12-slots #1, 399-1-0, rating 2802 (herd 2088, 344-56):
  exactly the local prediction (lbseeds3 399/400). New active set drops arlene-wheat;
  scripts/lb_seeds4.sh replays a challenger's games for it.
- "PPO" in these notes = JJ's agent ppo-v6n-it60 (Local-LB branch add-ppo-v6n-it60), used only
  as an opponent. We do no PPO/RL training (user decision, Sep 25).
- so2rw (sell_order 2 + return_wages): exact Local-LB replay (d603858 set) 399/400, +13,959 per game
  vs so2 +13,058 on the same 400 games: paired +901 (270 better, 116 worse). Packaged as
  submissions/sep25-bc-opus-v12-wages; replay vs slots (lbseeds4) and gates running.
- Network screen under the so2rw compiler, C++ mirror vs v12 (40 games each): v12_aug 18/40
  (-0.9k), v13_w384b 14/40 (-1.7k), v14_all 15/40 (-1.8k), v16_db 7/40 (-4.1k), v18_dbstrong 4/40
  (-3.9k), v12 seed-1 replicate 8/40 (-2.8k). v12 is the strongest network; replicates of its
  recipe (seeds 2-4, GPU) are screened the same way.
- full_care probe (feed+care every animal while care banks): 10-30, -1.3k vs so2rw: the network's
  selective care (top-player rates) is better. Off.
- CPU policy (measured, memory cpu-budget): i7-14700K 8P+12E; CPU time per game is flat at 4/8/16
  P-threads, so oversubscription (load ~60) adds no throughput, delays results and made BC training
  1.8x slower. Keep ~16 game threads, run decision-critical jobs first, pause bulk jobs
  (value data paused with SIGSTOP, PID in ps).

## 16:30 - branch 4 pushed: submit/pavel-bc-opus-v12-wages

- Package submissions/sep25-bc-opus-v12-wages (models/cand_so2rw: sidecar "sell_order 2",
  "return_wages 1"). Validator OK; Local-LB runner check vs slots identical to the dev build (6 games).
- vs slots on the exact Local-LB seeds (df4172a set): 28/40, +890. Gates: C++ 224/224 (larger margins
  than so2 vs all 7), frozen replays 136/145 (so2 135), zoo 192/192 (so2 190). Full lbseeds4 running.

## 16:40 - inference settings under the wages compiler; load leveling

- Mirror vs wages (so2rw), 40 games each, seeds 1300-1319: opening DSM 25-15 (+895), DECEM 20-20,
  Mother-Goose 23-17 (+291), Majkel 0-40 (-4.8k), none 10-30 (-1.5k), Vadim 3 days 17-23 (-716),
  Vadim 9 days 20-20 (-589); strength 100 25-15 (+750), 200 19-21 (+306), 250 18-22 (+485);
  v12 seed-2 network 19-21 (-1.5k).
- Confirmation on fresh seeds 1400-1419: DSM opening 31-9, +1.5k. (strength 100 and DSM+100 running.)
- Load leveling (user idea; CompileOptions::level_hires: on a day needing >= N hires, recompile once
  with harvests/collections that lose nothing by waiting moved to tomorrow, one hire fewer):
  level 12 -> 5-33-2, -2.9k vs wages. Tiles, not wages, are the binding resource: a harvest a day
  later delays the replant a day (about a third of a wheat cycle), worth more than the $144-233
  hire. Off; level 11 stopped.

## 16:55 - DSM opening confirmed; value model parked

- DSM opening (days 0-5 in DSM's style; model.bin.opening "6 7") vs wages (Vadim "6 8"): seeds
  1300 25-15 (+0.9k), 1400 31-9 (+1.5k), 1500 26-14 (+1.1k): 82-38. Candidate cand_dsm6 = wages
  compiler + DSM opening. Exact Local-LB replay (lbseeds4/dsm6) running.
- Not additive: strength 100 alone 25-15 / 24-16, but DSM + strength 100 19-21 (1400) and DSM +
  strength 100 + deadline 22 12-28 (1500), 16-24 (1600). Market deadline 22: 24-16 then 20-20
  (neutral); DSM + deadline 22 22-18 vs DSM alone 26-14 on 1500.
- v12 seed replicates: s1 8-32, s2 19-21, s3 5-35: v12 is an outlier; seeds stop after s4.
- On-policy value model (600 perturbed games vs 5 C++ opponents, value_v1 fine-tuned, valid R^2
  0.47): decision test unchanged (argmax V regret $3.4k vs baseline $1.4k). Next-dawn value does
  not rank decision pushes whose effects show days later. Parked.
- Opening screen vs cand_dsm6 (20 games each): DSM 3/9 days, DSM all game, 24 other styles.

## 17:00 - wages vs PPO settled; DSM on the Local-LB set; hire cap counts shed stock

- JJ's PPO agent, 80 paired games (seeds 1310-1349): wages 79/80 +13.9k vs so2 76/80 +13.3k,
  paired +662 (47 better, 24 worse). The 20-game -1.5k earlier was noise (PPO games swing +-14k).
- cand_dsm6 exact Local-LB replay (df4172a set): 382/400, +13,566/game (wages 386/400, +12,570);
  vs slots 24/40 (wages 28/40), herd 38/40 (40/40), robust 40/40 (38/40). Higher margins, not
  more wins; head to head vs wages 82-38 (C++ mirror).
- Opening screen vs DSM (20 games each): DSM 3/9 days, DSM all game and 24 other styles all
  worse or equal (best: YumeNeko 10/20 +1.0k, M&M 8/20 +0.3k, akmr 11/20). DSM 6 days kept.
- New failure mode (mirror seed 1409, -14k): day 10 ends with $14 cash and melons in the shed;
  day 11's pre-solve hire cap used dawn cash only (5 hires affordable), nothing routed, survival.
  hire_cap_stock: the cap also counts 90% of the sale value of sellable shed stock (our sales
  precede our hires in the order list; the hourly funding check still decides). Seed 1409:
  -14.4k -> -2.9k / -3.5k; no survival day. Defaults exact (8/8). Mirror vs DSM on 3 seed sets
  running. Survival days are rare (2-4 per 400 Local-LB games) but decide near-equal games.

## 17:05 - hire-cap fix neutral in normal games; opening models; seed 4 = v12

- hire_cap_stock vs DSM without it: 8-8-24 (1300), 10-10-20 (1400), 11-11-18 (1500), mean ~0:
  identical games except where a crunch occurs (rescue case seed 1409 -14k -> -3k). Adopt.
- Opening model sidecar (<model>.opening_model; model for days < opening days): zoo_dsm 16-24 (-90),
  zoo_majkel with Majkel style 9-31 (-3.1k) vs the DSM style pin on v12. Keep the pin.
- v12 seed 4 vs v12 (wages compiler): 20-20 (+169): the first replicate as strong as v12 (seeds 1-3:
  8, 19, 5 of 40). Seeds 5-7 queued on the GPU; s4 + DSM opening and a v12+s4 ensemble of the
  whole-farm heads (<model>.ensemble sidecar) vs cand_dsm6 running; mid-game style screen (days 6-29:
  DSM, Vadim, DECEM, Goose, M&M, YumeNeko, akmr) running.
- No late plantings (strawberry/melon after day 19, tomato after 21) in 400 Local-LB games.
- v12 fine-tuned 800 steps at lr 1e-4 on strength >= 50 (2,219 perspectives): screening.

## 17:10 - style schedule, networks, ensembles; herd-size gap on replays

- Mid-game style (days 6-29 via model.bin.style) with the DSM opening, vs cand_dsm6 (40 games):
  M&M 26-14 (+1.5k; confirming on 1400/1500), DSM 19-21 (-0.3k), Vadim 19-21 (-0.2k),
  YumeNeko 8-32 (-4.0k); DECEM, Goose, akmr pending.
- Networks with the DSM opening vs v12+DSM: seed 4 19-21 (-0.6k), strength>=50 fine-tune 15-25
  (-0.9k); ensemble of v12+s4 whole-farm heads 8-32 (-1.2k). v12 stays.
- Replay-gate loss 112471604 (-14.1k in Boey's opponent seat): the original player reached 18 animals
  (12 cows) by day 12 with an 80-90 unit shed at dawn; we stop at 14 animals from day 10 despite
  $9.9k cash on day 11 and $14.5k on day 12. BC closed-loop gap: in "small herd + lots of cash on
  day 11" states the policy does not catch up. Probe: <model>.decode sidecar (quantile of the
  new-animal total on given days); q_animal 0.7 days 8-14 / 6-12 and 0.6 days 6-14 vs cand_dsm6
  running. It is a policy bias, kept only if it wins clearly on fresh seeds and the Local-LB replay.

## 17:15 - style schedule closed; herd reach

- Mid-game styles rejected: M&M 26-14 / 17-23 / 16-24 (1300/1400/1500: noise), DECEM 19-21, Goose 7-33,
  akmr 7-33, YumeNeko 8-32, DSM 19-21, Vadim 19-21. Days 6-29 stay unstyled.
- q_animal 0.7 on days 8-14 (fixed push) vs cand_dsm6: 26-12-2 (+570) on 1300; confirming on 1400.
- Budget-aware herd reach (<model>.decode "reach_days a b", "reach q..."): on those days, after the
  plan compiles in full, try higher new-animal quantiles (0.8, 0.7, 0.6) and keep the first that
  also compiles in full (funded with the next-dawn reserve). Defaults exact. reach 6-14 and 8-14 vs
  cand_dsm6 running.

## 17:20 - evaluation caution: 20-seed mirror sets vary a lot

- DSM (cand_dsm6) vs wages on the Local-LB seeds 7000-7019 (wages will be opponent 7): 17-23, -890.
  Same games in the C++ mirror: 38/40 identical, so the harnesses agree; the seed set differs.
  DSM vs wages over all sets: 25-15, 31-9, 26-14, 17-23 = 99-61 (62%). The per-set spread is
  large (mirrored seats make 40 games worth ~20-30 independent outcomes): single-set screens carry
  winner's-curse risk. From now on: >= 4 seed sets (160 games) incl. the Local-LB seeds before
  adopting anything.
- Herd reach: days 6-14 (0.8/0.7/0.6) 29-11 (+499) on 1300; days 8-14 24-16 (-382); days 8-14 with
  0.7 only 24-14-2 (+59). Confirmation of reach 6-14 on 1400/1500 running.
- Herd reach days 6-14 (0.8/0.7/0.6) vs cand_dsm6: 29-11 (+499) on 1300, 25-9-6 (+1,111) on 1400.
  Replay gate (145 frozen top-team games): reach days 8-14 139/145 +50.3k vs DSM 134/145 +48.8k and
  wages 136/145 +47.7k: the herd-size gap seen in those replays is partly closed.
- Final evaluation running: vs wages (so2rw) on seed sets 1300/1400/1500/1600/7000 (200 games each):
  fin_C = DSM + hire_cap_stock, fin_D = fin_C + herd reach 6-14, fin_E = wages + hire_cap_stock +
  herd reach (Vadim opening).

## 17:25 - final evaluation vs wages (partial); collect-all ported

- vs wages (so2rw), C++ mirror, per seed set (1300 / 1400 / 1500):
  fin_C (DSM + hire_cap_stock) 26-14 / 31-9 / 26-14; fin_D (+ herd reach 6-14) 28-12 / 31-9 / pending;
  fin_E (Vadim + hire_cap_stock + herd reach) 16-24 / 23-17 / 26-14. 1600 and the Local-LB seeds 7000
  pending. Herd reach vs DSM alone: 29-11, 25-9-6, 32-8 (86-28-6).
- Pre-packaged fin_D as submissions/sep25-bc-opus-v12-reach (not pushed).
- Ported the other session's collect-all (CompileOptions::collect_all, sidecar key): collect every
  animal's fertilizer, also without another job that day (their mirror +1.7k). Defaults exact.
  fin_G = fin_D + collect_all vs fin_D queued (3 seed sets); fin_F = herd reach to day 18 vs fin_D queued.

## 17:35 - final evaluation vs wages: D strongest broadly, loses the Local-LB wages seeds

- vs wages, C++ mirror, seed sets 1300 / 1400 / 1500 / 1600 / 7000 (7000 = the Local-LB seeds of the
  wages pairing):
  fin_C (DSM + hire_cap_stock)          26-14 / 31-9 / 26-14 / 23-17 / 15-25 = 121-79 (60.5%)
  fin_D (fin_C + herd reach 6-14)       28-12 / 31-9 / 29-11 / 20-20 / 18-22 = 126-74 (63%)
  fin_E (Vadim + hire_cap_stock + reach) 16-24 / 23-17 / 26-14 / 17-21-2 / 22-18 = 104-94-2
  Python Local-LB replay of D vs wages on 7000-7019: 18/40, -676 (matches the mirror), max action 2.57 s.
- D is the strongest candidate on broad seeds but would likely not become Local-LB #1 (it loses the
  wages pairing on its fixed seeds). Not pushed. Next: make the agent stronger (collect-all G,
  blend forecast H, herd reach to day 18 F; each vs D on 3 sets), then re-check on all 5 sets.

## 17:35 - ports from the other session on top of fin_D

- vs fin_D (1300): collect_all (G) 28-12 (+916); forecast_blend (H) 35-5 (+5.2k); herd reach to day 18 (F)
  9-11-20 (-97, stopped). The blend gain is large in the mirror (the other session: +2.4k vs their
  base; neutral on a 160-game Local-LB panel), so it needs the third-party check too.
- Route search option (CompileOptions::route_search, sidecar key): 1 = every solve with 16 route
  variants, 8 workforce variants and opportunistic hire reduction (the other session's "routesearch",
  mirror +2.0k [+0.4k, +3.6k]); 2 = only on days needing >= 12 hires; off under time pressure.
  Defaults exact.
- fin_I = fin_D + collect_all + forecast_blend vs wages on 5 seed sets running; fin_J = fin_I +
  route_search 1 vs fin_I queued.

## 17:40 - Local-LB rules changed; fin_I vs wages

- Local-LB main 490c63d: wages merged (ranking rebuild pending); roster rule "top 30 from the full
  roster" (max_active_agents 30) and seeds_per_pair 7; new agent saitejabandaruin-mega-ensemble-3000.
  A challenger plays every active agent (alphabetical, k from 0) on seeds 1000*(k+1)..+6, both seats.
  scripts/lb_seeds_roster.sh replays that for a roster file; lb_seeds3/4/5 are obsolete.
- fin_I (DSM + herd reach + hire_cap_stock + collect_all + forecast_blend) vs wages: 33-7 (+3.4k),
  34-6 (+4.3k), 34-6 (+3.3k) on 1300/1400/1500; 1600 and 7000 pending. Blend alone on top of fin_D:
  35-5, 30-10, 36-4; collect_all on top of fin_D: 28-12, 29-11, 31-9.
- Pre-packaged fin_I as submissions/sep25-bc-opus-v12-forecast; branch worktree
  work/local_lb_prs/pavel-bc-opus-v12-forecast from 490c63d, validator OK (not pushed yet).

## 17:45 - branch 5 pushed: submit/pavel-bc-opus-v12-forecast (fin_I)

- fin_I vs wages, C++ mirror: 33-7, 34-6, 34-6, 28-12 (1600), 35-5 (7000) = 164-36 (82%), ~+3.2k/game.
- Package (submissions/sep25-bc-opus-v12-forecast) identical to the dev build through the Local-LB
  runner vs wages (6 games); overage use <= 0.7 s of 60 s; validator OK. Branch from 490c63d.
- Next on top of fin_I: route_search 1 (fin_J), slack_care (fin_K), Vadim opening (fin_Iv);
  third-party replay of fin_I (old 10-agent roster seeds) queued; exact replay once the rebuilt
  top-30 roster is known (scripts/lb_seeds_roster.sh).

## 17:45 - on top of forecast (fin_I)

- Rejected: slack_care (full care capped at the plan's hire count) 9-31 (-2.2k); Vadim opening instead
  of DSM 13-27 (-1.5k); v12 seed 7 3-37 (seeds 1-7: 8, 19, 5, 20, 16, 9, 3 of 40 vs v12).
- route_search 1 (fin_J) 23-17 (-153) on 1300; 1400/1500 running at low priority.
- Ported racedp (CompileOptions::race_dp: per-product market delivery hour chosen by the sale DP, tried
  before the fixed deadline; the wage check applies to both market attempts). Defaults exact. fin_L =
  fin_I + race_dp vs fin_I running (3 sets).
- Local-LB snapshot refreshed to 490c63d (all 68 agents + JJ's PPO; old snapshot in localLB_main_old).
  Full-roster panel: fin_I vs every agent, 7 seeds x 2 seats with the new seed rule (as if all active):
  reports/roster_all/fin_I.
- Lesson again: kill by exact PID after checking the command, never by a pattern that also matches
  monitor scripts (killed two monitors).

## 17:55 - route search timing; racedp neutral so far

- Compile time on the 8 recorded Local-LB games (loaded E-cores, 4 threads): fin_I 21.4 s/game (worst
  day 4.3 s); route_search 1 73.1 s/game (worst day 25.2 s): far over the 60 s overage budget, rejected
  despite 23-17 / 25-15 vs fin_I; route_search 2 (wide only on days needing >= 12 hires) 28.1 s/game
  (worst day 14.1 s): mirror vs fin_I queued; adopt only with a clear win and a Local-LB runner check.
- racedp (fin_L) vs fin_I: 21-19 (+266), 21-19 (+434); 1500 pending.
- Full-roster panel so far 190/190 wins (14 opponents).
- Replay gate (145 frozen top-team games): fin_I 143/145, +54.4k (wages 136/145 +47.7k; DSM 134/145;
  reach 139/145): the best so far vs top-team play.
- Crop reach (<model>.decode "creach_days a b", "creach q..."): like herd reach for the new-crop total,
  after herd reach (keeps its quantile). fin_N = fin_I + creach days 6-20 (0.7, 0.6) vs fin_I running.
  fin_I plays identically on the new build (opt_check 8/8 vs the old build).

## 18:00 - forecast robustness; racedp cost

- Full-roster panel (fin_I vs every Local-LB agent, 7 seeds x 2 seats): 444/447 after 32 opponents;
  losses only vs slots (12/14, +3.5k) and wages (13/14, +3.4k). Exact replay vs the current 10-agent
  roster (seeds 1000(k+1)..+6): 42/42 so far.
- racedp (fin_L) vs fin_I: 21-19, 21-19, 23-11-6 = 65-49-6 (+0.3k); compile 32.1 vs 20.9 s/game
  (worst day 8.0 vs 4.3 s, loaded E-cores). 1600 and 7000 queued before a decision.

## 18:05 - add-ons on forecast (fin_I)

- route_search 1 (fin_J): 23-17, 25-15, 25-15 = 73-47 (+0.6k) but 73 s compile/game: too slow.
  Timing (8 recorded games, loaded E-cores): fin_I 21 s (worst day 4.5 s), mode 2 (wide only on days
  needing >= 12 hires) 28 s (14 s), mode 3 (8 route / 4 workforce variants every solve) 46 s (16 s),
  mode 1 73 s (25 s). Mode 2 is the only affordable one: mirror vs fin_I queued.
- Crop reach (fin_N, creach days 6-20) 21-19 (+89); melon race on fin_I (fin_M) 22-18 (+382); more sets
  running. Blend visible weight 0.7 / 0.3 (fin_B70 / fin_B30) vs 0.5 running.
- Full-roster panel paused (SIGSTOP) to keep CPU for the exact current-roster replay and mirrors.

## 18:10 - add-on results on forecast (fin_I)

- Exact replay vs the current 10-agent roster (490c63d): 137/140, +13.8k (slots 11/14, others 14/14);
  added to the forecast branch README (commit 0b4ac2c).
- racedp (fin_L): 21-19, 21-19, 23-11-6, 21-15-4 = 86-64-10 (+0.3k); 7000 pending; +50% compile.
- Crop reach (fin_N, days 6-20): 21-19, 26-14 = 47-33 (+0.4k); 1500 pending.
- Melon race (fin_M): 22-18, 19-21 = 41-39 (-0.3k): dropped.
- Route search mode 2 (fin_J2): 19-13-8 (+0.2k); 1400 pending.
- Blend visible weight 0.7 (fin_B70) 27-13 (+0.3k) on 1300; 0.3 and 1400 pending; 0.85 and 1.0 queued.
- GPU: seed replicates 8-11 restarted (the waiting wrappers had exited: self-matching pgrep).

## 18:20 - bundle P promising; forecast accuracy audit; opponent models

- fin_P (fin_I + blend_visible 70 + race_dp + route_search 2) vs fin_I: 24-16 (+612), 31-9 (+1.4k);
  1500 and 7000 running.
- tools/forecast_audit (600 recorded on-policy games): MAE of the opponent's daily sales per product,
  trailing / blend / adapt / blend70 / blend+stock: carrot 1.03/0.88/2.32/0.95/0.94; tomato
  0.74/0.61/0.74/0.60/0.64; strawberry 5.48/5.71/7.01/6.24/5.49; melon 4.36/1.00/1.33/1.00/0.45; egg
  1.91/2.08/3.08/2.24/2.05; milk 5.05/4.84/5.40/5.14/5.19; wool 4.85/4.15/4.67/4.20/4.20 (mean sales
  per day 1.1/0.5/9.0/2.7/1.9/8.3/6.0). Blend is the most accurate overall; the adaptive sell-through
  model (forecast_adapt, History::sell_through) is worse everywhere; adding inferred stock helps melons
  a lot and strawberries a little, hurts milk. Strawberry, milk and wool stay poorly predicted by all.
- forecast_stock (fin_Q) 16-24 (+5) on 1300; forecast_adapt (fin_R) running. v12 seed 8: 7-33.
- Blend weight: 0.7 46-34, 0.3 37-43, 0.85 25-15 (1300); 1.0 pending.

## 18:30 - P not clearly better; overage-aware search options; expert iteration v2 started

- fin_P vs fin_I: 24-16, 31-9, 19-21, 15-25 (7000) = 89-71 (+0.55k). Not a clear enough gain for a
  new PR. Add-ons on top of forecast are at diminishing returns (each 0 to +0.5k, 50-60%): blend
  weight 0.85 42-38, adapt 40-40, stock 40-40, crop reach 60-60, race 41-39.
- Timing safety: CompileOptions::time_left (remaining overage, set by the agent each day): route_search
  only with > 40 s left, race_dp only with > 30 s. P in the Local-LB runner vs slots: overage left at
  game end 45-56 s (before: 36-52 s). Defaults and fin_P's decisions with ample time unchanged (8/8).
- Expert iteration v2 (scripts/expert_iter2.sh): search_games with exact copies of our Local-LB
  lineage (forecast itself, wages, so2), 7 decision pushes searched on days 0-12 with full-game
  rollouts, traces saved: reports/expert_iter2/<opponent>/ (40 games x 2 seats each, 4 threads each).
  Plan: build_search_corpus -> extract -> fine-tune v12 (init model.pt, lr 1e-4, arrays_v5 + search
  arrays repeated ~10x, --search-opening "6 7") on the GPU -> mirror vs fin_I.

## 18:30 - fitted opponent model

- tools/forecast_audit FORECAST_ROWS dumps (trailing, visible, stock, sell-through, day, actual) per game,
  day and product. LAD fit per product on 600 on-policy games (80/20 split by game), held-out MAE
  blend -> fitted: carrot 0.82 -> 0.23, tomato 0.58 -> 0.17, strawberry 5.72 -> 4.68, melon 0.99 ->
  0.34, egg 2.03 -> 0.51, milk 4.87 -> 3.86, wool 3.97 -> 3.41. The inferred shed stock is the missing
  predictor (coefficient ~1 for carrot, tomato, egg: opponents sell their stock daily).
- CompileOptions::forecast_fit (coefficients in compiler.cpp FORECAST_FIT, fitted on all 600 games).
  Defaults and fin_I exact. fin_S = fin_I with forecast_fit vs fin_I running (3 sets). Next: refit on
  games against our own lineage (expert-iteration traces) once they exist.
- Blend weight 1.0: 36-44 (worse). v12 seed 9: 11-29.

## 18:40 - opponent-adaptive forecast selection

- The fitted forecast (fin_S) vs blend in the mirror: 18-22, 20-20 (neutral). On games against our
  own agent (33 self-play traces) the fit is worse than blend for carrots and melons: our lineage holds
  stock instead of selling it daily, so opponents differ and one fixed model does not fit all.
- History now keeps per day the opponent's dawn stock, dawn visible supply, harvests and sales;
  visible_supply moved to history.hpp. CompileOptions::forecast_select: per product, blend or fitted,
  whichever had the smaller absolute error on this opponent over the last 3 days (online model
  selection). Defaults and fin_I exact.
- Offline MAE (C++/zoo opponents | self-play), blend / fitted / select: carrot 0.88/0.26/0.43 |
  1.82/1.95/1.60; tomato 0.61/0.17/0.34 | 1.15/1.17/1.13; strawberry 5.71/4.64/4.34 | 4.96/4.31/4.35;
  melon 1.00/0.34/0.69 | 1.41/2.24/1.55; egg 2.08/0.57/1.12 | 4.19/2.77/3.46; milk 4.84/3.78/3.57 |
  5.61/5.29/5.09; wool 4.15/3.56/3.62 | 4.82/4.77/4.75. Select is never the worst and often the best.
- fin_T = fin_I + forecast_select vs fin_I running.

## 18:45 - why a better forecast does not show in the mirror (user question)

- Own vs opponent money: blend vs trailing (fin_H vs fin_D) raised our own money (108.7k vs 103.5k,
  112.0k vs 107.1k); fitted vs blend (fin_S vs fin_I) changes neither side (105.3k vs 105.4k).
- The executor used the dawn forecast all day, never conditioned on the opponent's sales so far today.
  CompileOptions::forecast_intraday: remaining hours expect (forecast total - sold today) in the same
  hourly shape. fin_U (fin_I + intraday) and fin_V (+ select + intraday) vs fin_I running.
- Timing error (tools/forecast_audit, share of the opponent's daily sales predicted in the wrong
  6-hour bucket, C++/zoo opponents | self-play): melon 0.65 | 0.34, wool 0.56 | 0.21, milk 0.43 | 0.13,
  strawberry 0.35 | 0.14, carrot 0.27 | 0.01, egg 0.16 | 0.18. Against our own lineage timing and totals
  are already well predicted by the blend, so the mirror (vs our lineage) cannot show forecast gains;
  varied opponents can. fitted 55-65, select (fin_T) 17-23 in the mirror.
- C++ opponent panels (7 opponents x 32 games) for fin_I, fin_S, fin_T running (expert iteration paused
  with SIGSTOP, PIDs in the scratchpad ei_pids.txt).
- forecast_timing (hourly shape from all past sales of the product, History::hour_profile): timing error
  not better (C++/zoo: strawberry 0.35 -> 0.43, milk 0.43 -> 0.47, melon 0.65 = 0.65; self-play wool
  0.21 -> 0.46): opponents change timing during the game; rejected without games.
- Mirror: fin_U (intraday) 23-15-2 (+46), fin_V (select + intraday) 19-21 (-49): neutral, as expected.

## 18:50 - forecast variants in the mirror; anticipation stack ported

- Mirror vs fin_I (1300 / 1400): select (fin_T) 17-23 / 28-12 (+277 avg); intraday (fin_U) 23-15-2 /
  26-12-2 (+286); select + intraday (fin_V) 19-21 / 27-13 (+489). Mildly positive even in the mirror.
- Ported the other session's anticipation stack ("oppprior_steps": their mirror 30-0-2 +9.4k, frozen
  top-10 +2.1k): source/opp_prior.hpp (strong-player hourly prior by product and day bucket, copied),
  CompileOptions forecast_prior (melon/milk/wool: blend total, prior shape mixed 50/50 with the trailing
  shape), race_early (race_dp delivery from hour 6), race_steps (retry unroutable early deliveries 1..5
  steps later), melon_tie_now. Defaults, fin_I and fin_L exact. fin_W = fin_I + the stack vs fin_I running.
- C++ opponent panels for fin_I / fin_S / fin_T running (12 of 21 files).

## 18:55 - varied opponents confirm the forecast gains (answer to the user's question)

- C++ opponent panel (7 opponents x 32 games, paired vs fin_I): fitted forecast (fin_S) +1,209
  [-36, +2,629] (arlene +3.6k, investment_context +3.1k); online selection (fin_T) +658 [+163, +1,319].
  In the mirror (vs our own lineage) the same variants were neutral (fin_S 55-65, fin_T 45-35): the
  forecast gains show against varied opponents, whose sales our trailing/blend forecast misjudges,
  not against our lineage. Selection is positive in both beds.
- C++ panels for fin_U (intraday) and fin_V (select + intraday) running.

## 19:05 - C++ panel: intraday conditioning does not help

- C++ panel vs fin_I (224 games): intraday (fin_U) -69 [-1,342, +1,137]; select + intraday (fin_V) +157
  [-1,251, +1,373] (king_rc4 -3.7k) vs select alone (fin_T) +658 [+163, +1,319]. Intraday dropped.
- Anticipation stack (fin_W) mirror vs fin_I: 32-8 (+1.2k), 27-13 (-812) = 59-21 (+0.2k); C++ panel running.
- fin_Y = fin_I + forecast_select + the anticipation stack (no intraday): mirror (2 sets) and C++ panel
  running. Expert iteration resumed at nice 10.
- v12 recipe seed replicates finished: wins of 40 vs v12 (same compiler, DSM opening from s5 on):
  s1 8, s2 19, s3 5, s4 20, s5 16, s6 9, s7 3, s8 7, s9 11, s10 23 (-1.1k), s11 12. v12 is a
  top-decile draw; no replicate replaces it. GPU next: the expert-iteration fine-tune.
- Expert iteration v2 restarted: the first run produced no finished game in ~1 h (13 searched dawns x
  7 full-game rollouts = 30-60 min per game per thread) and both seats of a mirrored seed repeat the
  same game. search_games SEARCH_SEAT=<0|1> plays one seat per seed; now SEARCH_LAST_DAY=10, 40 games
  x 3 opponents, 4 threads each at nice 10 (~5 h).

## 19:30 - audit: cheaper evaluation; herd reach funded only under the expected forecast

- User direction: justify long CPU-heavy runs; iterate on fixes with few games, confirm only the best.
  - Stopped expert iteration v2 (3 x 4 threads, ~5 h, no finished game after 10 min). The first run's
    data was too small to move the network and the fine-tune path is untested; it restarts only after a
    cheap pilot shows a signal.
  - Stopped fin_W's C++ panel (fin_Y contains W's changes).
  - The mirror's two seats mostly repeat one game (1267 of 2360 seat pairs identical, the rest close):
    a 40-game set holds about 20-25 independent games. New scripts/mirror1.sh plays seat 0 only (half
    the CPU); screen on 1-2 sets, confirm on more sets, the C++ panel and the Local-LB seeds.
  - Diagnose big losses directly (3-12 targeted games) before running more sets.
- fin_Y vs fin_I on 1400: 29-11 but -685 mean: three games lost by $11-28k (seeds 1404, 1409, 1417),
  also in fin_W, in no candidate without the stack. Ablation (Y minus one option, 12 games, 76 s):
  without forecast_prior all three are won (+1.1k, +3.0k, +3.3k); race_early, race_steps and
  melon_tie_now change nothing.
- Root cause (seed 1404, DC10_DAYLOG day log, farm_audit): on day 6 the herd reach (q0.80) bought 6 geese
  and 1 cow on $1,090 cash with a plan funded only under the expected forecast (stress check failed).
  Same plan in both runs; with the prior's sale timing, day 6 earned $488 less, dawn 7 had $20, no wheat
  and no sellable output: survival planned 0 feeds and 7 animals escaped (-$28.6k). Without the prior
  the same plan ended with $281, just enough. The prior only exposed a fragile reach rule.
- Fix: decode sidecar "reach_stress 1": herd and crop reach keep a larger plan only if it is also funded
  under the stress forecast (the opponent sells all its visible output at hour 2). Y + reach_stress on
  the three seeds: +949, +1,335, -480 (was -28.6k, -11.5k, -24.5k). fin_I + reach_stress vs fin_I
  (one seat): 6-5-9 (+147), 7-6-7 (-62): neutral when nothing goes wrong.
- fin_Y C++ panel vs fin_I (224 games): +1,549 [+348, +2,682], 100% wins both (select alone +658):
  the stack helps against varied opponents.
- Tools: DC10_DAYLOG (agent: one line per dawn: money, status, fallback, hires, new animals, planned
  feeds, wheat bought, stress-funded, funding variant, reason); search_games and full_games CSVs gain
  escaped / rival_escaped (placed animals lost before day 25; only escapes remove one; later, unfed animals are often left to escape).
- Running: one-seat screen (Z = Y without the prior, Z+rs, Y+rs, I+rs; sets 1300, 1400), Y+rs C++ panel.

## 20:00 - screen results; 4th quadrant (user question); Q4 fine-tune

- One-seat screen vs fin_I (seeds 1300-1319 + 1400-1419, 40 games): I+rs 13-11-16 (+42), Z (Y without
  the prior) 38-2 (+2.9k), Z+rs 37-3 (+5.3k), Y+rs 37-3 (+2.7k). Head to head Y+rs vs Z+rs (60 games):
  23-37 (-0.9k). stress_down (Z+sd, base plan must be stress-funded on reach days) vs Z+rs seed by seed:
  14 identical, 14 better, 12 worse, median 0; its higher mean (+0.9k) comes from two fin_I collapses.
  Dropped.
- fin_I (pushed forecast agent) has the same day-6 herd-reach fragility: seeds 1302/1316 it reaches dawn
  7 with $17-31 and loses 10 animals. Mirror means vs fin_I are dominated by such collapses; count wins.
- C++ panel vs fin_I (224 games; both seats repeat one game, so 112 unique): Y+rs +1,890 [+561, +3,094],
  100% wins; Y+rs vs Y +342 [-250, +942]. Z+rs panel running.
- Early escapes (before day 25) after the fix: 1-4 of 32 games per opponent; Z+rs vs ahmed_v25 seed 700
  (won +67k): the network feeds 29-32 of 33-35 animals from day 19 and lets cows go while growing sheep
  17 -> 26 (yarn shop): a network trade-off, not a funding failure.
- Stopped a stale value-data job (scripts/value_data.sh, 8 threads, stuck 3 h on zoo_majkel).
- 4th quadrant (user question). Replays (work/q4_analysis, farm_audit + new tools/quadrant_audit):
  - The latest 3 submissions of DSM, DECEM, Vadim and mtmr buy Q4 in 95-100% of games (UMG 56%,
    Majkel 0%), on day 10 (median), funded by the day-10 melon sales (dawn 10: $400; dawn 11: $2.2k).
  - vs the same teams' older games without Q4: +4 geese, +5-7 wheat tiles, +4-6 tomato tiles, +3-6
    strawberry tiles; same cows, sheep, melons, workers. Wheat produced days 11-29 +120 units (600-660),
    of which 365-430 are sold: wheat is feed and a cash crop.
  - Q4 itself (quadrant_audit): mostly wheat (10-11.5 tiles by day 13), some strawberries/carrots, few
    animals, 4-9 tiles left empty; the 3rd quadrant takes the extra tomatoes and geese.
  - Ours (Z+rs): never buys Q4 (also with the DSM style on days 0-13 or all game: 18-2 +2.8k, 17-3
    +2.6k vs 19-1 +6.6k); $11k at dawn 11, 3 quadrants full (~75 tiles used); herd as large as DSM's
    Q4 herd; 6-10 fewer wheat and ~6 fewer tomato tiles than DSM with Q4.
  - Forced Q4 with v12 (decode sidecar "land_push a b bias", value probe): day 10 cannot be funded at
    hour 0 and the later-hour variants find no route (falls back to NoLand); day 11: 12-8 +1.3k vs base
    19-1 +6.6k. Degradations (1319, 1317): the $4k crowds out the day-12 herd reach (3 sheep vs 9), the
    new land gets 17-34 carrots, the shed sits at 81-100 from day 13, 4 cows escape (1317); the margin
    also moves with the shop lottery (25 more empty tiles reshuffle the night RNG).
  - Data: 1,851 top-15 perspectives own Q4 by day 12 in corpus_v7db (mtmr 433, DSM 426, DECEM 249,
    Vadim 195, Kaggledew 191, UMG 184, "Fourth Quadrant" 110); none is in v12's training data (v5).
  - Fine-tune v12_q4ft: v12 weights, arrays_v5 + those 1,851 games (55k dawns, x3) under a new style
    slot 31 (synthetic team 99000031 in data/styles.json), 10k steps, lr 1e-4 (3 min on the GPU).
    Variants: Q4a (DSM opening, style 31 days 6-29), Q4b (style 31 all game), Q4c (DSM opening, pooled).
    First 12 games: buys Q4 on day 11 in 2-4 of 12 games; own money vs Z+rs +9.0k / +3.1k / +5.9k.
