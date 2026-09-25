# How to train a value model for the BC + day compiler agent

Goal: the missing piece found in `BOUNDARY.md`: an estimate of what a dawn state is worth, so that
cross-day trade-offs (what to drop when over budget, how much stock to hold, cash kept) are chosen
by value, not by BC probability or fixed compiler rules. The compiler and the BC policy stay;
the value model is a new learned component used by both.

## 1. What the model predicts and how it is used

V(s) for a dawn state s (our seat's observation): expected final result from here.

Heads (one network):

| Head | Target | Loss | Use |
|---|---|---|---|
| margin | final own money - final opponent money, / 20,000 | Huber (delta 0.5) | ranking candidates |
| win | 1 if final margin > 0 (0.5 on a tie) | BCE | promotion checks, close games |
| own, opponent | final own money, final opponent money, / 50,000 | Huber | diagnosis (own economy vs opponent effect, see X5) |

Uses:

1. **Over-budget re-plan (first use).** The decoder proposes K budget-feasible intents (drop sets
   from its count distributions: min-loss covering knapsack, plus a few alternatives: drop a
   crop type, defer land, one animal fewer). Each candidate is compiled (the trim loop already
   compiles several variants); the compiler's day simulation gives our end-of-day state s'_a.
   Choose argmax V(s'_a). The opponent part of s'_a is copied from today's observation (the day
   simulation uses a dummy opponent); it is the same for all candidates, so it cancels in the
   ranking.
2. **Terminal values for the compiler (later).** Value of one more unit of product p, wheat,
   fertilizer or $100 of cash at night: finite differences V(s' + e_x) - V(s'). Use only after a
   sanity check (monotone, close to tomorrow's net price for products, >= 1 for cash early) on
   held-out states; otherwise keep the rule values.

## 2. Data

1. **Replay dawns (already extracted).** `experiments/v10/sep24_BC_opus/data/arrays_v5` (+ `v6x`):
   1.45M dawns with actor features (global 256, crop and animal groups, grids) and
   `meta.i64` = (episode, seat, day, team, submission, split, n_crops, n_animals). Join the final
   result per (episode, seat) from `data/perspectives_meta.csv` (`reward` = final own money,
   `margin`). No re-extraction is needed.
2. **Our own games (on-policy).** V must value states our policy reaches, under our continuation.
   Replays estimate value under top players' continuation. Generate 2,000-4,000 C++ games of the
   current agent (and X10) against the zoo and the C++ ports of Local-LB agents
   (`tools/full_games` with `BC_WRITE_TRACES`), extract dawns with the same `tools/extract`
   (works on any valid trace; expert-iteration traces extracted 600/600 days), label with the final
   result. Throughput: roughly 1,000-2,000 games per hour on 16 threads, load permitting.
3. **Perturbed games (coverage of alternatives).** Top replays barely vary on the decisions we
   want to value (every top team opens with 3 sheep). On a random 20-30% of dawns in days 0-12,
   replace the intent by an alternative (a different budget drop set, `BC_SAMPLE=1` quantiles, one
   entity more or fewer), then continue normally. This breaks the confound "richer players have
   more animals" and gives V examples of 2-sheep and 3-sheep farms in comparable states.
4. **Paired rollouts (counterfactual labels).** `tools/search_games` already plays every candidate
   from the same dawn to the end with copies of both agents; it keeps only the best. Log every
   candidate's final margin (a few lines in the candidate loop). The paired difference
   z_a - z_b from the same state, seed and opponent removes most outcome noise. Use about 200-500
   such dawns (budget days 0-9 first) as the **decision validation set**, and optionally as
   training data with a pairwise loss on V(s'_a) - V(s'_b).

Splits: by episode (both seats together; the `split` column), plus a time-forward hold-out (latest
replay days) and a hold-out of opponent families.

## 3. Variance: the main difficulty

Measured on 5,618 top-team perspectives: the final margin has std $11.3k; the current cash margin
explains R^2 = 0.00-0.01 of it on days 0-12, 0.26 on day 18, 0.57 on day 24, 0.83 on day 29; on
day 3 the cash leader wins only 41% (cash spent on assets wins). Early values must come from the
farm, and early outcome labels are very noisy. Countermeasures:

- Use all 1.45M dawns (the label is free) plus on-policy games.
- TD(lambda) targets after a Monte Carlo warm-up: y_d = (1 - lambda) V(s_{d+1}) + lambda y_{d+1},
  with y_29 = final margin and lambda about 0.8, from a slowly updated target network. This cuts
  variance for early days.
- Paired-rollout labels for decisions (section 2.4).
- An ensemble of 3-5 seeds; use the mean for decisions and the spread as an uncertainty flag
  (fall back to the BC order when the candidates are within the spread).

## 4. Inputs and architecture

- Same C++ features as the actor (`source/features.hpp`), computed by the same code at extraction
  and inference (native parity like the BC model). Everything needed is already there: day, own
  and opponent money, margin, land, shed, seeds, shops, per-farm counts, groups with ages and held
  product, market prices and inventories, inferred opponent flow.
- A source flag input (replay teacher vs our policy); at inference: ours.
- Separate network first (same encoder family as the BC model: group MLPs, mean/max/sum pooling,
  context MLP, then the heads), width 256. A shared trunk with BC can be tried later only if it does
  not hurt BC validation loss.
- Training like `scripts/train.py`: AdamW lr 1e-3 cosine, batch 1,024 dawns, strict FP32, 20-40k
  steps; one training process at a time (RAM: arrays take 24-32 GB).

## 5. Validation (in order; only the last two decide)

1. Calibration of the win head and margin R^2 by day on held-out episodes, against two baselines:
   current cash margin, and a linear model on farm counts (animals by species, plants, land).
2. Sanity of marginal values (section 1.2).
3. **Decision test:** on the paired-rollout set, share of candidate pairs ordered correctly and the
   regret of argmax V versus the rollout-best candidate, in dollars. Compare with the BC order
   (X10) and the compiler rule. Promote only if V's regret is lower.
4. **Full games:** V-ranked over-budget re-plans vs X10 on the Local-LB panel, the zoo panel and the
   C++ opponents (paired, seed-clustered CIs), and the latency tail (K compiles on about 4 days per
   game must fit the 60 s overage; use the fast search effort for candidate scoring).

## 6. Order of work

1. Join labels to the replay arrays; train the first V (Monte Carlo targets); step 5.1.
2. Add logging of all candidates to `search_games`; build the decision set on days 0-9 over-budget
   dawns; step 5.3 against BC order and rules.
3. Generate on-policy and perturbed games; retrain with the source flag and TD(lambda); repeat 5.3.
4. Wire V into the budget re-plan loop; step 5.4.
5. Only then: terminal values for the compiler and the night-inventory work
   (`OPPORTUNITY_night_inventory.md`).
