# Plan

Ordered by expected value per hour of work. Each item names the first experiment and the gate.
Working rules from Pavel for this pipeline: keep the deterministic day compiler; fix the wrong
assumption in the design (a concept), not the symptom, and use env-gated patches only to measure a
concept's value; move a decision from the compiler to the network only when the compiler fails at it
consistently for a conceptual reason; no end-to-end RL / PPO training of this agent (decided Sep 25;
expert iteration is the agreed way to learn from search). Evaluation rules: EVALUATION.md section 1.

## Now (hours)

1. **Get a newer agent onto Kaggle.** Only `robust` is submitted (score 2795, 50-14). `forecast` fixes
   several of its loss causes (fertilizer collection, wage-aware returns, 10 melons by day 3 instead of
   8, slot-ordered sales, visible-supply forecast) and beats `wages` 164-36, but has the herd-reach
   fragility (WEAKNESSES 0.5); prefer `z_rs` (item 2) or at least `forecast` + `reach_stress 1`.
   Package with `agent/package.sh <out> candidates/<name>`, check with `agent/play.py` (and a Kaggle
   kernel check with kaggle-environments 1.32.7 installed), then submit. Submission slots are limited
   per day: decide with the team which agent goes first.
2. **Confirm `z_rs` (fin_Z_rs) and push it.** It fixes `forecast`'s herd-reach fragility
   (WEAKNESSES 0.5) and adds online forecast selection and early race deliveries: one-seat screens 19-1
   and 18-2 vs `forecast`, 37-23 vs Y+rs, C++ panel +1.4k (222/224). Still needed: more mirror sets
   (1500, 1600, the Local-LB seeds of its pairing with `forecast`), the exact Local-LB roster replay, the
   frozen top-10 and Kaggle beds. Y+rs (`anticipate` + `reach_stress`) is the alternative with the best
   C++ panel (+1.9k [+0.2k, +3.7k]). If only one agent can go to Kaggle, `z_rs` is the candidate once
   confirmed; otherwise `forecast` with `reach_stress 1` added.
3. **Shed overflow on heavy days** (WEAKNESSES 3). Trace day 26 of the seed-1600 mirror game with
   `DC10_DEBUG`, find why 54 units were destroyed after an OK compile, and make tonight's deposit a
   hard constraint. Gate: discards < 1 per game on the C++ panel and the mirror, no margin loss.

## Next (days): conceptual compiler gaps

4. **Melons first** (WEAKNESSES 1). The sale side is done (anticipation stack); the routes are late.
   Concept: a ripe-melon day is a race that starts at dawn: the harvest-and-return trip must be the
   first job (farmer placed near ripe melons at the end of day 9, or melons planted on the tiles
   nearest the shed on day 0). First probe: force the melon trip first on day 10-11 and measure the
   first-sale hour on the Local-LB family (target <= 8) and on the 64 Kaggle games
   (`traces/kaggle/`, frozen-replay bed as in the companion's `frozen*.sh`).
5. **Plan inventory across the night** (WEAKNESSES 2). Design in
   `docs/weakness_analysis/OPPORTUNITY_night_inventory.md`: one capacity plan over tonight's deposit,
   tomorrow's dawn stock and tomorrow's first markets (a dawn sell-down makes room); held stock valued by
   the best of the next days' prices from known shop consumption and the opponent's expected supply,
   not 0.95 x tomorrow; returns costed by real wages inside routing. Evidence to beat: top teams keep
   65-90 units at dawn and sell half as much into crashes. Gates: Local-LB family (morning sellers),
   frozen Kaggle games, mirror; hard checks: discards < 1/game, unsold stock $0 at the end.
6. **Funding envelope + constrained decoding** (WEAKNESSES 6). The compiler reports cash available by
   hour (cash + certain income - wages - feed - inputs); the decoder returns the most likely intent
   inside it (min-loss covering knapsack over the count distributions, the same DP family as group
   decoding). Replaces trims. First probe exists: `DC10_BUDGET_REVISION` (units by network preference);
   the missing part is hour-0 cash timing (day 6 fails on land + hires + wheat at hour 0).
7. **Route construction** (WEAKNESSES 4): trip templates (shed -> animals near the shed -> crops ->
   deposit on the way back) and the Fibonacci wage of each extra hire in the route objective.
   Evidence: +13 moves per day, 50% vs 64% combined trips; the wide route search found better routes
   (73-47) but is 3.4x too slow, so the gain is in construction, not in more search.

## Later: learning

8. **Expert iteration** (search-distilled BC). Infrastructure: `scripts/expert_iter2.sh`,
   `build_search_corpus.py`, `ei2_finetune.sh`, `train.py --search-opening`. Search with exact copies of
   varied opponents (our lineage, C++ agents, zoo clones), one seat per seed, days 0-10; accumulate
   thousands of changed days before fine-tuning (the first 239 were too few). Gate: mirror + C++ panel +
   exact Local-LB replay; the network must win with the compiler unchanged.
9. **Value model with on-policy paired data** (WEAKNESSES 7). Replay-trained V does not rank our own
   alternatives. Train on paired rollouts of our own decision alternatives (the search runs save them:
   `SEARCH_ROLLOUTS`), validate with `scripts/value_decision_test.py` before using V anywhere (candidate
   ranking at dawn, terminal values for held stock and cash in the compiler).
10. **Networks**: train several seeds of the v12 recipe (seed variance is large: 3-23 of 40 vs v12) and
    select by games; try cross-group coordination (decisions of earlier groups as inputs) and
    whole-farm autoregressive decoding; keep the v5 corpus (more replay-DB data hurt).
11. **Opponent model**: learn whether this opponent holds in a crash and when it sells (price- and
    hour-conditioned), select per opponent online (`forecast_select` is the first step). Judge against
    varied opponents, never only in the mirror.

## Keep doing

- Replay the exact Local-LB games before every push; predict the table.
- Report own money, opponent money and margin separately; keep discards and unsold stock in every report.
- Update `evidence/` style result files per run (one CSV per run, both seats, seeds recorded).
