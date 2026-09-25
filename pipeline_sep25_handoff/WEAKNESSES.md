# Weaknesses

Ranked by how much they cost against strong opponents, with the evidence and the status of fixes.
Sources: the 14 Kaggle losses of `robust` (all vs top-50 players), traced losses of `forecast`, the
companion weakness study (`docs/weakness_analysis/`: REPORT, GAPS, BOUNDARY, KAGGLE_REPLAYS,
OPPORTUNITY_night_inventory, ROTATION_LOG) and the experiment's own audits.

## 0. Where the agent stands

- Against the Local-LB and every committed agent it almost never loses (962/966); its losses there
  are against our own lineage. Against the Kaggle top 50 it loses often: `robust` went 50-14 on
  Kaggle with all 14 losses against ranks 5-49 (score 2795; top 10 is 2917-3065).
- Per loss, ours minus the opponent (mean over 14): melons -5.2k (negative in 93%), milk -3.3k (93%),
  fertilizer -3.0k (86%), wages -1.9k (86%), eggs -0.9k; strawberries +3.2k and crops +3.5k for us.
  Full table: `docs/weakness_analysis/KAGGLE_REPLAYS.md` ("All 14 losses in one table").
- `forecast` (not submitted) already contains fixes for part of this (collect all fertilizer,
  wage-aware returns, herd rules with 10 melons by day 3, slot-ordered sales, visible-supply forecast).
  Its size of improvement on Kaggle is unknown.

## 0.5 Known fragility of the pushed `forecast` agent (fixed in candidates)

Herd reach (days 6-14) keeps a larger new-animal plan if it compiles in full, and "in full" includes
plans funded only under the expected forecast. When sales come in a little lower, the next dawn has no
cash, no wheat and nothing to sell; survival plans no feeds and animals escape (mirror seeds 1302/1316:
dawn 7 with $17-31, 10 animals lost; `anticipate` lost three games by $11-28k this way). Fix: decode key
`reach_stress 1` (in `z_rs`), neutral when nothing goes wrong. The deeper gap is section 6: budget
decisions checked only for today, with a fixed reserve rule for tomorrow.

## 1. Melons: we sell after the opponent

- Kaggle: the opponent sold melons first in 12 of 14 losses; our first melon sale was at hour 23 of
  day 10 in 34 of 36 early games (theirs hour 9). Melons have no shop demand, so the first seller of a
  harvest wave takes the top of the price curve and nothing recovers: $185 vs $224 per unit.
- Cause (compiler): harvested melons stay in the worker's hands until the evening deposit (returns
  are due by hour 20 and nothing values an earlier melon deposit); with no opponent melon history
  every hour ties in the sale DP and ties went to the latest hour; the trailing forecast predicts no
  opponent melons before its first sale.
- Status: the visible-supply forecast (`forecast`) and the anticipation stack (`anticipate`: hourly
  prior, DP-chosen delivery hours from hour 6, melons sold on arrival) moved the median first sale to
  hour 10-11; the Local-LB family (the Kaggle public plan) sells from hour 9, so we still sell first in
  0% of those games. Planting more melons (the public 2-sheep/12-melon opening) lost 4.6k.
- Missing: being in the shed by hour 8 on day 10 (a routing routine for ripe-melon days: melons on the
  tiles nearest the shed, or the farmer ending day 9 next to them).

## 2. One-day horizon: selling contested products late or into crashes

- Top teams keep 65-90 units in the shed at dawn and sell 27 units per day at hours 0-5 (ours 11), after
  the overnight price recovery; they hold strawberries through a crash (sekai013: 54 units at $123-190
  after the crash; we sold 60 units at $41-90 during it). In Kaggle losses they sold 52% of milk at dawn
  (we 26%). We dump at hours 22-23 to empty the shed (46 wool at $26 in one loss).
- Cause (compiler): the sale DP ends at night; stock kept overnight is worth a fixed 0.95 x tomorrow's
  price as if all were sold at once tomorrow; returns are sized so tonight's deposit fits an almost empty
  shed. A multi-day price recovery is invisible.
- Status: open. Bolted-on holding failed: sell-fraction cap -23.6k, fixed overnight stock -13.2k (shed
  overflow), hold factor 1.0 -3.8k, dropping evening returns -1.8k (then the opponent sells first).
  Design: `docs/weakness_analysis/OPPORTUNITY_night_inventory.md`.

## 3. Heavy days overflow the shed

- Traced mirror loss of `forecast` vs `wages` (seed 1600 seat 0, `traces/cpp_games/`): day 26 compiled
  with status OK and no fallback, but 154 units were carried into the night and 54 were destroyed
  (11 wheat, 8 tomatoes, 8 eggs, 6 milk, 14 wool, 7 fertilizer). The same pattern appeared in replay
  continuations of top-player farms (workers carrying 200+ units, 100+ destroyed).
- Cause for this game: not traced yet. The return ladder tries market + capacity returns, then the
  capacity part alone, then 75/50/25% of it, each as a whole re-solve; the accepted plan is not
  checked against tonight's actual deposit, and the room estimate assumes the planned sales happen.
  First step: rerun `full_games bc:<wages> 1600 1 1` with `DC10_DEBUG=1` and read the `return L..`
  lines of day 26 (carried, room, wanted, percent, status).
- Fix direction: treat tonight's shed capacity as a hard constraint of the plan (staged return targets
  that follow the base schedule's carried inventory, or a post-route deposit pass), and part of gap 2.

## 4. Labour: walking and wages

- `robust` on Kaggle: $7.5k wages per game vs top-10 teams' $5.4k; 13 hires on 28% of mid-game days
  (top 3.5%). Core cause (companion labour ledger): walking, +13 moves per day (0.55 workers) and fewer
  combined animal+crop trips (50% vs 64%); 40% from same-day returns, the rest route construction.
  End-of-day idle turns (hours 20-23) are spare capacity.
- Status: `return_wages` (in `wages`, `forecast`) removed the worst returns (+0.9k). Wider route search
  won 73-47 but costs 3.4x compile time (73 s per game, over budget). Open: route construction with
  trip templates (shed -> animals -> crops -> deposit) and the Fibonacci wage of each extra hire in the
  route objective.

## 5. Herd size and composition

- Frozen top-team replays: top players reach 18 animals by day 12 while we stopped at 14 with $10-15k
  idle cash. Kaggle losses: fewer geese (5 vs 7, 7 vs 11) or cows (8 vs 11) in 4 of 6.
- Cause: BC closed-loop drift (the network is unbiased on expert states but, in "small herd + lots of
  cash" states it does not catch up) and the compiler's budget rules.
- Status: herd reach (days 6-14, `forecast`) 86-28-6; reach beyond day 14 did not help. Crop reach
  neutral. In its 140 exact Local-LB roster games `forecast` has 19.5 animals at day 12 (10 melons at
  day-3 dawn in every game), so the size gap is closed there; composition against specific opponents
  (geese vs cows) is not studied. The 4th quadrant (bought by top-10 teams in 34% of games) is never bought: the land logit
  learned from teams that mostly stay at 3 quadrants (the latest top submissions buy it on day 10 in
  95-100% of games); forcing it or fine-tuning on Q4 games lost, mainly because the shed overflowed
  (135 units destroyed per game): it depends on sections 2-3.

## 6. Budget-blind intents

- The DayIntent has no budget. On investment days (0, 6, 7, 9: 91% of trimmed units) the network asks
  for more than the day can fund and the compiler cuts by a fixed rule (day 0 crops first, else the
  most expensive entity first). Day 6 typically asks for $3.2k of animals and land against $1.1k cash;
  failures there are about hour-0 cash timing, not the total.
- Status: rules fixed the worst case (herd). Letting the network choose what to drop (least likely
  units first) was +5.3k before slot sales and neutral after. Budget revision as a covering knapsack
  works mechanically but the dollar shortfall is the wrong abstraction.
- Design: the compiler reports a funding envelope by hour, and the decoder returns the most likely
  intent inside it (same DP family as the group decoding).

## 7. No value estimate anywhere (root cause of 2, 5, 6)

BC gives probabilities, not values; the compiler values cross-day effects with fixed constants (hold
0.95, next-dawn reserve, 1 wheat per animal, trim order, $2 per returned unit). A replay-trained value
model predicts outcomes (R^2 0.4-0.6 on days 3-9) but ranks our own alternatives worse than random.
Per-dawn search with an exact opponent copy triples the margin, and a wrong opponent model keeps a
quarter of that gain: there is large headroom in the network's decisions that nothing captures yet.

## 8. Network limitations

- Whole-farm fields are decoded independently (except total x shares); no cross-group coordination
  (a group does not see other groups' decisions); no memory of earlier days beyond the inferred
  opponent flow; the one-shot option loss dominates training.
- Game strength varies strongly between training seeds at equal validation loss (3-23 of 40 wins vs
  v12); v12 is a lucky draw, and no offline metric predicts which seed is strong.
- Style pins help only for the opening (days 0-5); every mid-game pin was noise or worse.

## 9. Things checked and not weak

Shop adaptation (same response as top teams), tile placement and layout compactness, land count (82%
of top games stay at 3 quadrants), crop yields per plant (ours higher), feeding and care rates (the
network matches top players; forcing full care lost), input-stock drift, time use (<= 2.8 s of the 60 s
overage per Kaggle game).
