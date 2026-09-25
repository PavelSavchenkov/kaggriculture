# Behavior cloning for Kaggriculture: status and open issues (Sep 25, 2026)

This document is self-contained. It describes a behavior-cloning (BC) policy
that plans one farm day at a time for the Kaggle game Kaggriculture, how it is
trained and evaluated, what went wrong, what was fixed, and what is still open.
The goal is to get advice on improving BC training and design.

## 1. The game in brief

- Two players, one 10x10 farm each, 30 days x 24 hourly turns (the last day has
  23 turns). Final score: own cash minus opponent cash (margin).
- Start: $3,000, one owned quadrant of the farm (25 tiles); three more can be
  bought ($1,000, $2,000, $4,000).
- Crops: wheat, carrot and melon are one-shot (harvested once); tomato and
  strawberry are ongoing (produce every few days). Crops need watering (two dry
  nights kill them); fertilizer doubles yield growth for a few days; late crops
  decay into weeds.
- Animals (goose, cow, sheep) live in coops/pastures, must be fed wheat, can be
  cared for (bonus production), and produce eggs/milk/wool that must be
  collected; they also produce fertilizer. Unfed animals escape.
- Workers: one farmer plus hired hands (Fibonacci wages, reset each day). Each
  unit does one action per hour (move, plant, water, harvest, pick up, drop...).
- Shared market: prices fall as players sell into it and recover through town
  and shop demand. Up to 10 market orders per turn (sales, purchases, hires).
- Shed holds 100 items. Items workers carry at the end of the day are deposited
  at night; anything above 100 is destroyed.
- Only public information is observable about the opponent (its board, cash,
  market effects), not its private inventory.

## 2. Architecture: network intent + deterministic day compiler

Every dawn the network emits one `DayIntent`: what the farm should accomplish
today. A deterministic day compiler turns it into all hourly worker actions and
market orders (routes via a separate route solver, purchases, hires, returns to
the shed, and an hourly sale optimizer). The network never picks tiles, paths
or sale orders.

`DayIntent` fields (all integer counts unless noted, 0-100):

| Scope | Fields |
|---|---|
| Whole farm | new wheat / carrot / tomato / strawberry / melon counts (planted today and alive after tonight); new goose / cow / sheep counts; unplaced goose / cow / sheep after tonight (animals kept in the shed); buy next land today (bool) |
| Each one-shot crop group (wheat, carrot, melon) | 9 counts that partition the group: the 8 combinations of {water, fertilize, harvest} plus `clear` (dig without harvest) |
| Each existing ongoing crop group (tomato, strawberry) | retain after tonight, clear today, fertilize today (<= retain), harvest today; abandon = size - retain - clear |
| Each animal group | feed, care (<= feed; care only with feed), collect |

Groups: crops/animals that are identical except for tile (same type, age,
yield/held product, dry days, remaining fertilizer days; animals: species, age,
held product, unfed days, care bonus) form one group; groups are ordered
lexicographically; new groups (one per crop type and species) follow. A dawn
has typically 5-40 groups (max 110).

Fixed values: in some states a field or option has only one sensible value
(e.g. day 29: no new entities; options with harvest are impossible on a crop too
young to harvest; watering a crop that decays today is worthless). These are
decided from the dawn state, get no loss, and decoding never emits anything
else. Constraints linking fields (partitions sum to the group size, care <=
feed, fertilize <= retain, new animals + kept animals >= animals in the shed at
dawn, enough free tiles) must be enforced by losses and decoding.

## 3. Data

- Labels: public Kaggle replays of top-20 leaderboard teams. Each replay day of
  one player (a "perspective-day") is replayed in an exact engine and converted
  to its DayIntent label. Conversion never guesses: days it cannot represent
  are reported (a coverage gate). Result: 112,020 perspective-days from 3,734
  perspectives (2,224 older + 1,510 downloaded fresh on Sep 24); 112,017
  represented, 3 ignored (a discarded animal), 0 failures; 32,289 individual
  actions dropped because fixed-value rules exclude them.
- Split by whole episode (both players together) with a hash: 80% train, 10%
  validation, 10% test (test not yet used). Train: 3,021 perspectives (~90k
  dawns); older-only experiments used 1,800 (~54k dawns).
- Teams are mixed: ~20 teams, several submissions each; strategies differ
  (animal-heavy, crop-heavy, melon rushes, etc.).

## 4. Inputs (actor features, causal only)

All features are computed by one C++ header shared by dataset extraction and
the live agent (native C++ inference matches PyTorch: 300/300 identical
decisions).

- Global (256 floats, ~208 used): day and days left; own/opponent cash (log),
  margin; land owned and next land price; shed contents; seeds; shops present;
  per farm (own, opponent) tile-kind counts, plants by crop, harvestable held
  crop units, animals by species, held animal units, fertilizer ready, empty
  coops/pastures, mean shed distance of producers, "producers ready within
  0/1/2/3 days" per product; per product market price, market inventory,
  expected opponent daily net sales (inferred from the last 3 days of market
  changes), demand from shops/town; hires affordable; money relative to land
  price.
- Crop group (48 floats): crop one-hot, ongoing flag, age, age relative to first
  and last yield day, yield/held, max yield, dry days, fertilizer days, decaying,
  new-group flag, size (linear and log), the 9 option fixed-value masks, ongoing
  survive/die/fertilize-fixed flags, product price, value (yield x price), days
  to decay or next production, mean shed distance, harvestable flag, days left.
- Animal group (32 floats): species, age, held, unfed, care bonus, new flag,
  size, care-fixed, held==0, product price, held value, at-capacity flag, days
  to next production, mean shed distance, days left.
- Grid (optional): own and opponent 10x10 boards, 24 channels (tile kind, crop,
  species, age, yield, dry days, fertilizer, harvestable, held, care bonus, shed
  distance, shed access).
- New-group sizes are unknown until the whole-farm counts are decoded: in
  training they come from the label (teacher forcing); at inference from the
  decoded counts. New groups are not pooled into the context.

## 5. Model

- Encoders: 3-layer MLPs (width W, ReLU) for global, crop groups, animal groups.
- Pooling over existing groups: mean, max and sum/10 per group type.
- Optional tile CNN: two 3x3 conv layers (32 channels) shared by both farms,
  mean+max pooled (128 floats).
- Context: 3-layer MLP over [global, crop pools, animal pools, grid] -> W.
- Whole-farm head (linear from context): 11 count fields x 101 classes, land
  logit, plus a factorization: total new crops (101 classes) + crop-type shares
  (softmax over 5), total new animals (101) + species shares (3).
- Group heads (3-layer MLPs) get [group encoding, context, "stage A" = the 12
  whole-farm values (label in training, decoded at inference)].
- Width 256 (~1.9-2.3M parameters). AdamW lr 1e-3, weight decay 1e-4, cosine
  schedule, batch 512 dawns, gradient clip 1.0, strict FP32, 6k-14k steps
  (~2-6 minutes on one RTX 5090). Best-validation checkpoint kept.

## 6. Output and decoding variants tried

| Version | Group outputs | Group decoding | Whole-farm decoding |
|---|---|---|---|
| v1 | one probability per field (target k/n), options softmax | round(n x p), largest remainder | argmax per count field |
| v2/v3 | same | same | median; or total (median) x type shares |
| v5 | 101-way count per field, sequentially masked by the members still unassigned (teacher forced), no "remaining" input | greedy in order, last option takes the rest | total x shares |
| v6 / v8 | sequential decoder: one MLP call per field, input adds field one-hot, values already decided, size, legal maximum (as in our earlier strongest BC) | greedy in order | total x shares |
| v7 / v9 | marginal: 101-way count per field over 0..size, independent | exact MAP by DP: maximize summed log-probabilities subject to the partition sum (and retain+clear <= size) | total x shares |

## 7. Evaluation

- Teacher-forced decode check: decode on replay dawns, compare field totals
  and one-shot member mismatch with labels.
- Replay continuations (fixed-opponent diagnostic): from a replay dawn (day 20,
  25 or 29) our agent plays one seat to game end against the opponent's
  recorded actions; score = our final margin minus the replay's final margin,
  on 211 validation perspectives (episode-clustered bootstrap CIs).
- Full games against `agent_sep23`, our previous strongest BC agent (its own
  network and compiler), seeds 600-631, both seats (64 games; seats of one seed
  often mirror each other, so treat as ~32 independent games).

## 8. Results

Full games vs agent_sep23 (64 games):

| Version | Compiler trim fallback | Wins | Mean margin |
|---|---|---|---|
| v1 argmax counts | no | 0/32 (seeds 500-515) | -32,231 |
| v1 median counts | no | 6/32 (seeds 500-515) | -9,883 |
| v2 (features v2, width 256) | no | 14/64 | -18,062 |
| v2 | yes | 42/64 | +4,867 |
| v5 count heads, greedy | yes | 52/64 | +12,984 |
| v5 heads + MAP DP (heads not trained as marginals) | yes | 0/64 | -133,765 |
| v6 sequential decoder | yes | 49/64 | +10,297 |
| v7 marginal heads + MAP DP | yes | 58/64 | +15,711 |
| v8 sequential + fresh data | yes | 54/64 | +16,393 |
| v9 marginal heads + MAP DP + fresh data (selected) | yes | 64/64 | +20,585 |

Selected model v9 on seeds not used for selection (700-731, both seats):

| Opponent | Wins | Mean margin |
|---|---|---|
| agent_sep23 | 61/64 | +19,216 |
| two_random_shop_league_v179 (older in-house league agent) | 62/64 | +49,948 |
| one_shop_no_geese_league_winner_v1 (older in-house) | 64/64 | +75,934 |

Full games are the main gate. Not yet measured against Local-LB (Python) or
current public top agents. In full games v9 discards 2.8 units per game (agent_sep23:
65), has 0-0.06 uncompiled days and 2.7-3.1 fallback days per game; the worst
single dawn compile takes 3.7 s.

Replay continuations (margin vs replay, 211 validation perspectives):

| Version | From day 20 (10 days) | From day 25 | Day 29 |
|---|---|---|---|
| v1 argmax | -4,047 | -1,725 | -240 |
| v1 median | -3,272 | -2,155 | -240 |
| v2 | -1,599 | -1,701 | -211 |
| v5 | -2,992 | -2,003 | -301 |
| v6 | -2,414 | -1,676 | -315 |
| v7 | -2,621 | -1,820 | -312 |
| v8 | -2,700 | -2,302 | -367 |
| v9 on the untouched test split (348 perspectives) | -2,342 | -1,839 | -223 |

For scale: the compiler alone, given the replay's own DayIntents on replay
dawns, is within about +-$50 of the replay per day on day 29 and loses about
$50/day on ordinary days, so most of the continuation gap is intent quality
plus compounding. Earlier notes put agent_sep23 at about -12,900 (from day 20)
and -5,300 (from day 25) on a different cohort.

Validation loss (lower is better; totals are not comparable across output
designs): v2 34.8, grid alone 34.9 (no gain), v5/v6 27.5-27.6, v8 (fresh data,
different validation set) 27.95. Sanity checks: train and validation errors
match (no overfitting at width 128; width 256 overfits after ~8k steps); the
network memorizes 512 dawns to 15.0 against an exact entropy floor of 14.3.

Teacher-forced field totals (validation, per dawn, predicted/label): new crops
9.2-9.4 / 9.8 on days 10-19 (under-planting remains), new animals 0.2-0.3 / 0.4
on days 10-19, harvest, feed, water close to labels; one-shot member mismatch
6% (days 0-9) and ~15% (days 10-28).

## 9. Issues found and fixed

1. Argmax decoding of whole-farm counts: when 0 is the single most likely count
   but most mass is on 10-25, argmax says 0. Under-planting. Fixed with median
   and total x shares.
2. Group fields as one per-member probability trained on k/n and decoded as
   round(n x p). This models an expected fraction, not a count. Experts act on
   whole groups 84-98% of the time (feed 92% all-or-none, care 98%, retain 97%,
   ongoing harvest 84%, one-shot group fully on one option 68%), so rounding a
   mixed probability yields partial actions the experts never take. Fixed with
   101-way count categoricals.
3. Sequentially masked count heads have untrained logits for counts the teacher
   path never allows (the last option had 0 remaining almost always). Exact DP
   decoding over them emptied fields into `clear`: 0/64 games. Fixed by training
   marginal heads for DP (v7) or by feeding remaining/assigned counts to a
   sequential decoder (v6/v8).
4. Compiler fallback (largest full-game effect): an early day that could not be
   funded dropped all new crops and animals, idling the farm for days. Now new
   entities are trimmed one unit at a time.

## 10. Open problems and hypotheses

- Continuations still lose $1.6-2.6k over 10 days vs top-player replays. Partly
  compiler: on heavy harvest days workers carry 200+ units into the night and
  the route solver cannot schedule the end-of-day returns, destroying 100+ units
  (continuation discards ~3,900 vs ~1,000 for the replay). Partly intent quality.
- Continuation and full-game rankings disagree (v2, with fractional group
  decoding, is best in continuations; v7-v9, with count decoding, are far better
  in full games). Count models harvest whole groups as experts do, which on heavy
  top-player farms hits the compiler's end-of-day return limit.
- Whole-farm fields are decoded independently (except total x shares); no
  coupling between new crops and capacity (free tiles) beyond the compiler.
- No cross-group coordination: each group's decision does not see decisions
  already made for other groups (our earlier strongest BC fed a running summary
  of decided counts and predicted output to later groups and to the whole-farm
  decoder, and decoded whole-farm fields last, autoregressively, with a
  free-tile mask).
- Mixed teachers: ~20 teams with different strategies; no style conditioning
  or single-strong-player fine-tuning tried yet in this pipeline.
- No temporal memory beyond the inferred opponent flow (no GRU over past days).
- Loss weighting: one-shot options dominate the loss; no per-head weights.
- The grid CNN did not help validation loss.

## 11. Questions for advice

1. Best structure for decoding exchangeable-member group counts with partition
   constraints: sequential autoregressive (with remaining counts as input),
   marginal heads + exact MAP DP, or joint sampling with beam search? How to
   make whole-farm and group decisions consistent (autoregression order)?
2. How to reduce multi-modality from mixed teachers (style tokens, team
   conditioning, filtering to one strong team, mixture heads)?
3. Validation loss and play strength disagree; what offline metric best predicts
   compiled play (e.g. decode-check field errors, intent distance weighted by
   economic value)?
4. Is attention over group tokens (instead of mean/max/sum pooling) likely to
   help, given that group decisions interact (harvesting frees tiles for new
   crops; feeding uses wheat)?
5. How to use closed-loop data (DAgger-style relabeling is hard without an
   expert oracle) to reduce compounding error in continuations?

## 12. Update (Sep 25, 02:00-05:00)

Evaluation now: Local-LB (official Kaggle engine; the 10 active agents reduce to 6
behaviour groups with identical games, e.g. #1 = #2), 8 extra Python agents
(public-notebook candidates, teammates' PPO/RL), 7 C++ agents, 145 frozen replays of
current top-10 Kaggle teams, and a league of our own BC versions. Seed-clustered 95%
CIs; paired comparisons on identical games.

Current best: v12_cond, 865-game panel vs v11: wins 87% -> 92%, margin +20.2k ->
+22.3k, paired +2.1k [+5, +4.5k]; Local-LB 79% -> 88%.

What changed and what we learned:
1. Compiler: made deterministic (wall-clock budgets removed); new entities must fit
   free tiles (design rule; enforced in validation and decoding; coverage gate passes on
   1.45M expert days); day compile time bounded (worst game ~6 s overage).
2. Data 13x: official daily datasets (every episode from Aug 16 replays exactly in our
   engine) + Meta Kaggle ratings: 48k perspectives, 1.45M days. Ladder ratings drift by
   ~300 points over weeks, so quality = rating minus the ladder's top-episode p90 on the
   same date; used as a conditioning input (not a filter) with the replay date.
3. More data + conditioning helped (v12). Filtering to the top 10% by strength was much
   worse (-11.3k at equal steps). Training longer is a big lever (10k vs 40k steps:
   -4.8k; Local-LB 57% vs 88%); validation loss tracks game strength.
4. Team style one-hot: index 0 had become "all unknown teams" (weaker), -6.6k when used;
   the all-zero style input works; fixed in training (unknown -> separate index,
   dropout -> no style).
5. Decisions matter most: uniform pushes of totals/land/service all lose, but choosing
   the best of 10 push settings per game wins 64/64; per-dawn search with exact
   opponent copies (days 0-12, 7 candidates) triples the margin (+11.9k -> +37.2k vs
   king_rc4). Open: how much survives without an exact opponent model.
6. Opening is a large lever: the DSM (Kaggle #1) style for days 0-5 then pooled:
   Local-LB 81% -> 97% (mini screen); M&M's opening: 81% -> 34%.
7. Single-team fine-tunes (3k steps) of Majkel (#4) and Vadim (#5) beat v12 head to
   head (13/32, 14/32 wins for v12).
8. Deviations from top players in closed loop: ~13 vs ~20 animals mid-game (fewer geese
   and sheep), half the wheat sold, land a day early; our per-animal output and sale
   prices are better. The network is unbiased on expert states (teacher-forced totals
   match), so the gap is closed-loop (openings, compiler safety rules).
9. The next-dawn cash reserve is necessary mid-game (without it one seed collapses:
   all cash spent on day 6, animals escape by day 11).

Revised questions: how to select/compose per-phase policies (opening vs mid-game) without
overfitting the evaluation panel; best way to turn search-improved games into training
labels (expert iteration) when the opponent model is imperfect.
