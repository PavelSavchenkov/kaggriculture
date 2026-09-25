# Kaggle replays of our submission (Sep 25, 16:45)

Submission 56553038 `pavel-bc-opus-v12-robust` (v12 network, Vadim opening days 0-5, trim0 compiler:
day-0 crops-first trims + next-dawn reserve from day 0). Submitted 14:32 UTC; public score 2075.9
after 18 public episodes. Team "My second life" is rank 690 (2371.3, ttyn's notebook). Top 10:
2917-3065.

Pipeline (`kaggle_ours/`): `list_subs.py` (our submissions), `fetch.py <submission id>` (episodes,
replays, engine traces, `meta_<id>.csv`), `overage.py <id>` (time use), `list.txt` →
`tools/ledger2` → `ledger/kaggle_*`; `q48.py` (per game, ours minus opponent by product),
`q49.py` (hires and wages per day, missed fertilizer), `compare.py 'ledger/top?' ledger/kaggle
ledger/x10lb` → `kaggle_ours/compare.txt`. The engine replays all 18 games to the exact Kaggle
rewards.

## Results

18 games, 18 wins, own $116.4k, opponent $73.9k, margin +$42.5k (closest: +$3.9k). The opponents
are low-rated (14 of 18 play the same plan: 12 melons by day 3, 17 animals by day 12). No loss yet,
so the findings below are process gaps, not loss causes; losses will come as the rating rises.

| Metric | Ours (Kaggle) | Top-10 teams | Ours (Local-LB, X10) |
|---|---:|---:|---:|
| Melons planted by day 3 (day 0 / day 1) | 7.9 (3 / 4) | 10.5 | 10.0 (6 / 4) |
| Melon units sold / price | 55 / $186 | 73 / $201 | 65 / $172 |
| Wages per game | $7,464 | $5,446 | $7,066 |
| Days 10-28 with 13 hires | 28% | 3.5% | - |
| Wage per day, days 10-28 | $366 | $263 | - |
| Fertilizer not collected (units/game) | 22.8 | 11.0 | 28.0 |
| Sold at hours 0-5 / 18-23 | 201 / 1,190 | 626 / 577 | 245 / 1,071 |
| Idle worker-turns | 7.7% | 5.4% | 7.9% |
| Animals at dawn day 9 / 12 / 20 | 11.9 / 17.4 / 18.0 | 13.0 / 18.0 / 18.8 | 12.4 / 17.0 / 17.7 |

Per game, ours minus opponent (`q48.py`): melon revenue is negative in 16 of 18 games (mean
-$7.5k; opponents sell 84 melons at $211, we sell 55 at $186); wages negative in 14 of 18. We win
on strawberries, tomatoes, fertilizer, wool and milk.

## Weaknesses, in order of cost

1. **Day-0 melons trimmed (about -$4k per game against melon planters).** The trim0 compiler's
   day-0 reserve trims 3 of the 6 day-0 melons (local X10 plants 6 on day 0, the submission 3 in
   all 18 games). Day-0 melons are the first wave (days 10-11, $230-260). Opponents with 12 melons
   sell at hour 0 of day 11. We sell at hour 23 of days 10, 11 and 12, so from day 11 on we sell
   after their dawn dump (for example, 113322012: the opponent sold 96 melons at day 11 hour 0 and
   we got $92 at hour 23). Most of the gap is quantity (29 units); the rest is price. Fix: already
   in the main session. so2 (`submit/pavel-bc-opus-v12-slots`) uses herd rules (reserve from
   day 1, network-ordered trims): 10 melons at day-3 dawn in all 400 of its exact Local-LB games
   (`experiments/v10/sep24_BC_opus/reports/lbseeds3/so2`). A melon race default would help further
   against these dawn sellers (Local-LB X2: +2.4k).
2. **Wages (+$2.0k per game against top teams).** Same pattern as locally: 13 hires on 28% of
   mid-game days, from same-day market returns. Fix: the main session's `return_wages` (mirror
   +646, Local-LB +1.8k) is not in the submission.
3. **Fertilizer not collected: 23 vs 11 units per game (about -$0.7k).** About 1 per day from day
   12 and 3.4 on day 28. Cause: the compiler adds a fertilizer collection only to animals that
   already have a feed, care or product job that day. Collecting every available fertilizer
   (`OPUS_COLLECT_ALL`, uses idle end-of-day capacity) wins the C++ mirror by +1.7k [+1.0k, +2.5k].
4. **Late selling (timing index wool 0.94, milk 0.97, strawberry 0.98 vs top 1.10, 1.06, 1.01).**
   74% of our units sell at hours 18-23. This does not cost us against these opponents (our
   absolute prices are higher), and every holding probe lost (mirror `hold100` -3.8k). Not a fix
   target.

Checked and fine: time use (at most 2.8 s of the 60 s overage per game; overage is used only at
dawn on days 18-26, up to 1.6 s over the 1 s limit), no discards, no escapes, no collapses, feeding
and care at top level, 3 quadrants like 82% of top games.

## Recommendation for the next submission

Submit so2 + `return_wages`: it fixes weaknesses 1 and 2 (the two biggest), and both are already
validated locally. Refresh this analysis when the submission meets 2300+ opponents: `fetch.py
56553038` again (cached replays are skipped), then rerun the ledger and `q48.py`.

## Refresh 17:14: 28 games, 28 wins, public score 2696.3

The rating rose from 2075.9 to 2696.3 (the team's best submission now). Recent opponents are rank
85-142 (2610-2690 on the 16:40 board); margins +6k to +12k. Closest game: 113343110 vs Oshbocker
(rank 531) +$175: our money was typical ($105.6k), the opponent had its best game of the sample
($130k revenue: strawberries $34k, wool $21k, milk $19k, wages $3.9k).

Most Kaggle opponents play one plan family: 12 melons by day 3, 17 animals at day 12, about 340
fertilizer units sold ($16-19k), wages about $4k. Our five Local-LB agents play exactly this
plan (12 / 17 / 330 / $4.8k), so Local-LB is a good proxy for these games. Per game against this
family (ours minus theirs): melons -$8.3k (their melon revenue is $17,165 in 20 of 28 games, the
same deterministic dawn sale on day 11), fertilizer revenue -$6k to -$13k (they sell all of it; we
use about 200 units on crops), wages -$3k to -$4.5k, milk and wool -$2k to -$4.5k; strawberries
+$2k to +$20k, tomatoes, wheat and eggs carry us.

## First loss (17:27): episode 113352523 vs sekai013 (rank 27, 2813.7), 83,196 vs 87,579 (-4,383)

31 games, 30 wins; the score had reached 2722.6 after 30 wins (opponents up to rank 58). The loss
is against a different plan from the public family: 12 melons, 5 animals on day 1 (2 cows + 3
sheep), 13 animals from day 10 (fewer than our 15), 29 strawberries, dawn inventory 76-99 units.

The game was even until the last dawn (ours $74.8k vs $75.5k at day 29 dawn; within $2.3k every
dawn from day 12 on). Where the $4.4k went (ours minus theirs, whole game):

| Item | Ours - theirs | Cause |
|---|---:|---|
| melons | -6,130 | 8 melons (trim0 day-0 trim: 3 day-0 melons instead of 6) vs their 12 |
| wages | -2,631 | $6.6k vs $4.0k; 13 hires on 5 days |
| strawberries, days 24-29 | -6,626 | they held 16-32 strawberries in the shed through the price crash and sold 54 at $123-190 ($9.4k); we sold 16 ($2.7k) |
| strawberries, days 18-23 | +2,668 | we sold 124 units at $12-108 ($6.5k), they sold 55 ($3.9k) |
| wheat revenue | -3,942 | offset by $1.5k less wheat bought |
| eggs, tomatoes | +6,010, +3,215 | 8 geese vs their 4; more tomatoes |

The decisive pattern (strawberry endgame): on days 19-20 both sides cleared strawberry plants and
dumped the harvest; the price fell to $12-52. Sekai stopped selling strawberries on days 21-23,
kept 18-32 in the shed (their shed was full of carrots and strawberries), and sold 7 at $123 (day
24), 25 at $173 (day 26) and 22 at $190 (day 29, above the $120 base). We kept selling into the
crash (31 at $41, 5 at $70, 24 at $90 on days 21-23) and had nothing when the price recovered.
Holding those 60 units (average $63) for ~5 days and selling at the recovered $160-190 is worth up
to about +$6k before our own price impact; a realistic +$3-4k would have decided the game (-$4.4k).

Why our compiler cannot do this: the sale DP values a held unit as if all held stock were sold at
once around noon tomorrow (0.95 x that price). A multi-day recovery (+$20-40 per day after a
crash, from shop consumption) is invisible, so selling at $41 today beats "all at once tomorrow".
Earlier holding probes raised the hold value uniformly (hold 1.0, sell caps) and lost; this is a
different decision: hold through a crash when the forward price, from known shop consumption,
rises for several days.

How common (`q54.py`, units sold on a day when that product's price reaches >= 1.5x within 5
days; upper bound on revenue given up per game): top teams 3.8k (milk 27, strawberries 21, wool
26 units per game); ours Local-LB 6.1k (39, 38, 30), Kaggle 5.7k, self-play mirror 10.3k (42, 49,
51). Top teams sell into crashes about half as much as we do.

Fixes:
1. Melons and wages: already in so2 + return_wages (not yet submitted).
2. Multi-day hold value (new, gap 3): value held units at the best of the next K days' noon prices
   (shop consumption, opponent's expected sales, optionally our own future output), capped at the
   last day, discounted 0.95 per day; the shed-capacity charge still decides what fits. Probe
   `OPUS_HOLD_DAYS=5` (+ `OPUS_HOLD_OWN`) in the mirror now.

Refresh 17:40: 34 games, 33 wins, score 2777.8. After the loss: wins vs arutyunoff (rank 42)
+7.6k, Driz Lo (67) +12.2k, syouya tobita (76) +20.4k.

## Second loss (17:48): episode 113357353 vs Artem The Farmer (rank 15, 2891.7), 82,538 vs 88,400 (-5,862)

35 games, 33 wins. Unlike the first loss, this one was decided by day 13 (their melon days put
them $6k ahead; the gap stayed $3-6k to the end). Ours minus theirs over the game:

| Item | Ours - theirs | Cause |
|---|---:|---|
| eggs | -3,578 | 5 geese vs 7: on day 6 both bought land; they added 4 geese and ended the day at $521, we added 1 goose + 2 cows and ended at $1,933 (we carried ~$1.4k more cash through days 6-8) |
| milk | -2,441 | same units (138 vs 139), price $47.5 vs $64.7 |
| melons | -2,574 | 11 melons vs 13; they sold 36 on day 11 in lots at hours 10-18 ($235-249) before our 24 at hour 23 ($221), and 36 on day 12 before our 6 |
| fertilizer | -1,720 | 17 fertilizer collections missed vs 2 (animals without a feed job; `OPUS_COLLECT_ALL` fixes this) |
| wages | -1,521 | $7.2k vs $5.7k |
| wool | +735 | 96 units vs 78, but $131.8 vs $152.8 per unit |
| strawberries, carrots, tomatoes | +2,554, +1,217, +773 | |
| wheat | net +600 | they sold $4.3k more but bought $4.9k more |

Contested markets (milk, wool) against a dawn seller: Artem sold 44 milk at hours 0-5 for $129
(after the overnight recovery) while our milk went at $14-88 later in the day; our wool went 53
units at hours 22-23 for $107 while they sold across the day at $134-192. Our 74% evening selling
loses these markets to a top opponent that sells first after the price recovers.

## Third loss (17:54): episode 113358537 vs ymg_aq (rank 32, 2796.6), 94,260 vs 101,417 (-7,157)

36 games, 33 wins, score 2760.7. Report: `loss_report.py 113358537`. Even until day 13; they pulled
ahead on days 14-17 and the gap grew to $8k.

| Item | Ours - theirs | Cause |
|---|---:|---|
| milk | -7,763 | 8 cows (from day 7) vs 9-11 (they kept buying cows to day 19): 230 units vs 279; they sold 192 at dawn for $131 |
| wages | -3,203 | $8.0k vs $4.8k; 13 hires on 10 days |
| melons | -2,848 | 12 plants each; they sold on day 10 at hours 10-16 ($249-270) before our 18 at hour 23 ($245), and on day 11 at hours 1-9 before our 24 at hour 23 ($198) |
| carrots, wool | -2,212, -1,243 | we dumped 46 wool at hours 22-23 for $26 |
| wheat | net +1,487 | they sold $4.8k more but bought $6.3k more |
| tomatoes, eggs, strawberries | +3,221, +2,528, +1,538 | |

## Fourth loss (18:03): episode 113360882 vs feles99 (rank 49, 2766.9), 91,456 vs 95,072 (-3,616)

39 games, 35 wins, score 2760.6. We led until day 16; they pulled ahead from day 17 (4th quadrant,
more wheat and eggs).

| Item | Ours - theirs | Cause |
|---|---:|---|
| eggs | -3,958 | 7 geese vs 10-11 |
| fertilizer | -3,330 | 29 collections missed vs 1 |
| melons | -3,145 | 8 melons (3 on day 0) vs 10; on day 10 we sold nothing (day revenue $1,154 vs $9,573): the day-0 melons were sold on day 11 at hour 9 ($235) after they had sold 36 from day 10 hour 9 ($245-265) |
| wheat | net -3,196 | they sold $4.6k more (4th quadrant, +$4k land) and bought $1.4k more |
| wages | -1,265 | $7.1k vs $5.8k |
| strawberries, milk, carrots | +4,142, +1,696, +1,099 | |

## Losses 5 and 6 (18:20): 43 games, 37 wins, score 2753.2

Loss 5, episode 113363196 vs SpaTaro (rank 28), 97,793 vs 100,421 (-2,628): melons -3,952 (they
sold 55 on day 10 from hour 6, before our 18 at hour 23); milk -5,432 (they ran 14 cows by day 13,
0 geese; we had 10 cows + 3 geese + 3 sheep); carrots -6,110; fertilizer -3,334 (14 missed);
wages -1,595; wheat net +3,259 (they bought $17.9k of wheat); strawberries +8,244, eggs +4,994.

Loss 6, episode 113365502 vs Sida Zuo (rank 35), 97,916 vs 102,836 (-4,920): melons -8,739
(they sold 47 on day 10 at hours 10-13 before our 18 at hour 23; our 24 on day 11 at hour 23);
eggs -5,879 (1 goose vs 4); wages -2,700; fertilizer -2,727 (32 missed); tomatoes -2,019;
strawberries +4,074, wool +2,364, wheat net +4,212.

Tally over the six losses (ranks 15-49), ours minus theirs:

| Cause | Losses | Range per game | Fix |
|---|---|---|---|
| melons sold after the opponent (and fewer day-0 melons) | 6 of 6 (9 of 9 incl. losses 7-9) | -2.6k to -9.1k | melon program (sell first; count on day 0 via so2); anticipation probe in test |
| wages | 6 of 6 | -1.3k to -3.2k | return_wages (so2rw) |
| missed fertilizer | 5 of 6 (14-32 units) | about -1k to -3.3k | `OPUS_COLLECT_ALL` (confirmed on three beds) |
| herd composition (fewer geese or cows) | 4 of 6 | -3.6k to -7.8k in the product | so2 herd rules; opponent-specific mixes differ |
| contested milk/wool sold late, strawberries into crashes | 4 of 6 | -1k to -7k | opponent anticipation (hourly prior), open |

The first three rows are fixable now and together exceed the margin in five of the six losses.

## Loss 7 (18:22): episode 113366873 vs DECEM (rank 5, 2991.9), 104,227 vs 124,045 (-19,818)

45 games, 38 wins, score 2761.4. First game against a top-5 team. Not a race loss but a volume
loss: their revenue $158.7k vs ours $130.0k; wages equal ($7.1k vs $7.4k).

| Item | Ours - theirs | Cause |
|---|---:|---|
| wool | -11,132 | 10 sheep from day 9 vs our 8; wool stayed near base ($197) all game (they sold 168 at dawn) |
| wheat, tomatoes, carrots | -6,854, -5,528, -1,794 | they bought the 4th quadrant (land $7k vs $3k) and grew more |
| melons | -3,124 | sold after them again (their day-10 sales at hours 9-17, ours at 23) |
| milk | -345 | we had 10 cows vs 7 but sold at $65.8 vs $98 (38 units into crashes at $22-25) |

## Losses 8 and 9 (18:47): 53 games, 44 wins, score 2790.2

Loss 8, episode 113370351 vs Otter Vibe (rank 22), 118,129 vs 118,306 (-177): melons -9,106 (they
sold 42 on day 10 at hours 8-11; we sold 3 at hour 11 and 15 at hour 23), fertilizer -5,515, eggs
-5,110 (3 geese vs 6), tomatoes -6,762; carrots +6,335, wool +4,768, strawberries +4,508, wheat net
+10k (they cycled $297k of wheat through the market: bought and sold, net +$0.4k).

Loss 9, episode 113372658 vs sekai013 (rank 27, second loss to them), 119,433 vs 122,232 (-2,799):
melons -7,870 (they sold 32 on day 10 at hour 16 and 18 on day 11 at hour 16, before our hour-23
sales), tomatoes -3,824, carrots -2,822, strawberries -2,333, fertilizer -2,055; wool +5,734,
wheat net +3,838; they bought the 4th quadrant.

In both, the melon gap alone exceeds the margin. Melons are now a factor in 9 of 9 losses.

## The 4th quadrant (user question 18:21: should we buy it, what stops us?)

- Top-10 teams buy it in 34% of games, almost always on day 11 (10th-90th percentile: days
  11-13). Teams differ: Boey, Kaggledew, Majkel never; Mother-Goose 13%, M&M 39%, Vadim 43%,
  DECEM 56%, DSM 69%, mtmr 89% (`q57.py`, 1,506 top-10 perspectives).
- Does it pay? Own money with vs without 4th quadrant, same team: M&M +14.9k, Mother-Goose +5.3k,
  mtmr +0.7k, DECEM -0.8k, Vadim -3.8k, DSM -8.1k; margin +3.0k (with) vs -0.2k (without) overall.
  A plan-level choice that fits some strategies, not a free gain (25 more tiles for $4,000 on day
  11 = more work, i.e. more hires, our weak point; and the right crops for the shops).
- Our Kaggle games: opponents with 4 quadrants beat us in 2 of 8 games, with 3 quadrants in 5 of
  36. No clear signal at this sample size.
- What stops us: the network. `buy_land` = land head logit + `BC_LAND_BIAS` > 0; it learned from
  teams that mostly keep 3 quadrants, so it never fires after the 3rd. The compiler would fund it
  ($6-8k at day-11 dawn in our games). Probe `OPUS_Q4_DAY=11` (buy on days 11-13 if at 3
  quadrants) queued: mirror and frozen replays.

## What the four losses have in common

All four were against top-50 players (ranks 15, 27, 32, 49); our money was low ($83k, $82.5k, $94k, $91k).
Recurring causes, with per-game cost in the three losses and the fix status:

1. **Melons sold after the opponent (all four: -$2.6k to -$6.1k).** No shop consumes
   melons, so the sale DP sees a flat price all day; with no opponent melon history on days
   10-12 every hour ties and ties go to the latest hour; returns are due by hour 20. Opponents
   harvest at dawn and sell from hour 0-10. Count mattered in losses 1-2 (8 and 11 melons vs
   12-13; robust's day-0 trim). Fix: sell melons as soon as they are in the shed (ties -> now),
   harvest ripe melons first, opponent's ripe melons as its forecast; so2 for the count. Probes
   running (frozen Kaggle replays: `melonrace`; `OPUS_MELON_TARGET` for the count).
2. **Wages (all four: -$1.3k to -$3.2k).** 13 hires on 5-10 days per game. Fix: return_wages
   (so2rw), wider route search.
3. **Contested animal products sold late or into crashes (all four).** Opponents sell milk,
   wool and strawberries at dawn after the overnight recovery, or hold through crashes; we dump
   at hours 22-23 (loss 3: 46 wool at $26) and into crashes (loss 1: strawberries). Open: needs
   an opponent model of when strong players sell.
4. **Herd composition (losses 2-4): fewer geese (5 vs 7, 7 vs 11) or cows (8 vs 11).** so2's herd reaches
   top level; the robust agent carried $1.4k more cash than the opponent on days 6-8.
5. **Missed fertilizer (losses 2-4: 17, 14, 29 units; loss 4: -$3.3k alone).** `OPUS_COLLECT_ALL` (mirror +1.7k, zoo +1.1k).

## Melon timing in all 36 Kaggle games (17:58)

Our first melon sale is at day 10 hour 23 in 34 of 36 games; the opponent's first melon sale is at
day 10 hour 9 in 22 games (hours 8-16 in most others). We sold melons first in 4 games. Average
melon price: ours $185.5 on 57 units, opponents $223.5 on 80 units (-$38 per unit, about -$2.2k per
game before the lost market share). Our day-0 melons are ripe at dawn of day 10 (first harvest
age 10, yield capped at 6, which we reach); no shop consumes melons, so the first units sold get
the top of the price curve and nothing recovers. Selling them by hour ~8 would put us first in
most games. Probes: melons due at hour 8 or 10 with ties -> now for melons only (frozen replays
queued).

Melon audit (`tools/melon_audit.cpp`, all 36 games; user question "why is it so hard to plant and
harvest melons early, it is a fixed routine"):

| | ours | opponents |
|---|---:|---|
| melons planted day 0 / day 1 / day 2+ (per game) | 3.0 / 4.1 / 1.9 | 10.2 / 0.7 / 0.9 |
| shed distance of melon plants | 3.93 | 3.99 |
| first melon harvest on day 10 (median hour) | 4 | 5 |
| first melon sale on day 10 (median hour) | 23 | 9 |

Planting position and harvest time are already fine. Two things are wrong:
1. After the harvest: the harvested melons stay in the worker's inventory until the evening
   deposit (market returns are due by hour 20 and nothing values an earlier melon deposit), and
   then the sale DP sells at hour 23 (no shop demand for melons, no opponent melon history, so
   every hour ties and ties go to the latest hour). Harvest at hour 4 + ~4 tiles back = melons
   could be in the shed and sold by hour 8-9, ahead of most opponents.
2. The count on day 0: opponents plant ~10 melons on day 0 (ripe on day 10, the top of the price
   curve); we plant 3 (robust's day-0 trim; so2 6) and most of ours on day 1 (ripe on day 11,
   after the opponents' 60 units). The opening (days 0-5) is decoded with the Vadim style, whose
   day-0 melon count is 6-7 before trims.
A scripted melon routine is well defined: on days when melons are ripe, harvest first (already),
deposit right away, sell on arrival; and put the melon budget on day 0.

## Day-0 melon budget of the Kaggle opponents (18:53)

Median Kaggle opponent on day 0 (45 games): 12 melons ($960), 2 cows + 2 sheep ($1,800), wheat
$350 (partly resold), hires $12; ends the day at $15. Ours: 3 melons + 2 cows + 3 sheep (robust;
so2 6 melons). They trade the third sheep ($500, about $1.0-1.6k of wool over the game) for about 6
more day-0 melons ($480 of seed, about 36 more units on day 10 at the top of the melon curve,
roughly $7-8k before the price impact). Top-10 teams open with 3 sheep and ~10 melons by day 3, so
our network (trained on them) keeps the sheep. Next opening probe: "2 cows + 2 sheep + 12 melons on
day 0" against the Local-LB family and frozen Kaggle replays; the running `melon12` probe raises
melons to 12 by day 2 but keeps the third sheep (extra melons are trimmed first when cash is
short).

## End-of-day dumping (19:23, 53 games)

Units sold below 30% of base price per game, ours / opponents: milk 51.9 / 63.5, wool 29.7 / 52.8,
strawberries 39.7 / 69.8 (the Kaggle public family dumps more than we do overall). But ours are
concentrated at hours 22-23 (16.6 milk, 19.3 wool, 10.6 strawberries vs their 4.7, 6.2, 5.1): the
evening sell-down under the night shed-capacity charge (losses 3-4: 46 wool at $26, 42 at $31).
About $2.2k of our revenue per game comes from these units. Part of gap 3 (one-day horizon); the
shed-capacity plan across the night is the design fix (`OPPORTUNITY_night_inventory.md`).

## All 14 losses in one table (19:35; 64 games, 50 wins, score 2795.1; `q58.py`)

Ours minus opponent per loss. `crops` = wheat + carrot + tomato revenue minus wheat bought; `land`
= land cost difference (+4000 when the opponent bought the 4th quadrant); `fmiss` = our missed
fertilizer collections; `mel_first` = hour of the first melon sale on the first melon day, ours /
theirs; `mel_n` = melon plants at dawn of day 3, ours / theirs; `herd12` = geese/cows/sheep at day 12.

```
       ep            opp  rank  margin  melon   milk   wool   egg  straw  ferti  crops  wages  land  fmiss mel_first mel_n           herd12
113366873          DECEM     5 -19,818  -3124   -345 -11132 -1121    632    559 -11119   -338  4000     23      23/9  8/10 5/10/8 vs 6/7/10
113357353 Artem The Farm    15  -5,862  -2574  -2441    735 -3578   2554  -1720   2572  -1521     0     17     23/10  8/13   5/6/3 vs 7/6/3
113386834 We wanna be to    16  -5,670  -6942  -4130   3366  4202    483  -5108   4276  -3574     0     24     23/23   8/4  5/7/3 vs 3/10/3
113374980 Azat Akhtyamov    18  -4,618  -5946  -3429  -2970     0   5727  -2621   5306   -791     0     30     13/10  8/12 0/12/3 vs 0/13/3
113380929 Azat Akhtyamov    18    -985  -4894  -4529  -1746  1734   4774  -3045   9156  -2100     0     11     23/10  8/12   5/6/3 vs 4/7/3
113379764        IsaiahP    20  -7,633   1346 -13990   2467 -5608  10346  -9363   6267  -2870     0     29     23/23  10/0 10/3/3 vs 13/4/3
113370351     Otter Vibe    22    -177  -9106  -1239   4768 -5110   4508  -5515   9568    461     0     28      11/8  8/10  3/4/10 vs 6/5/8
113377396       marwar22    23  -6,392  -9240  -1678  -1049 -6825    477  -1645  15636  -4286     0     22     23/11  8/10  3/3/11 vs 8/3/8
113352523       sekai013    27  -4,383  -6130  -1512  -1278  6010    103  -1526   2292  -2631     0     38      23/9  8/12   8/4/3 vs 4/6/3
113372658       sekai013    27  -2,799  -7870  -1185   5734  -255  -2333  -2055  -2602    518  4000     15      23/9  8/10 4/3/10 vs 5/6/10
113363196        SpaTaro    28  -2,628  -3952  -5432   -971  4994   8244  -3334  -2851  -1595     0     14      23/6  8/12 3/10/3 vs 0/12/2
113358537         ymg_aq    32  -7,157  -2848  -7763  -1243  2528   1538     62   2496  -3203     0     14     23/10  8/10   6/8/3 vs 5/9/3
113365502       Sida Zuo    35  -4,920  -8739   -752   2364 -5879   4074  -2727  10695  -2700     0     32     23/10   8/9  1/7/11 vs 4/6/8
113360882        feles99    49  -3,616  -3145   1696   -732 -3958   4142  -3330  -2556  -1265  4000     29       9/9  8/10  7/7/3 vs 11/6/3

mean over losses:
```

Mean over the 14 losses: melons -5.2k (negative in 93%), milk -3.3k (93%), fertilizer -3.0k (86%),
wages -1.9k (86%), eggs -0.9k (57%), wool -0.1k (57%); strawberries +3.2k and crops +3.5k in our
favour. Melons: the opponent sold first in 12 of 14 (our first sale at hour 23 in 11); we had 8
melons by day 3 in 13 of 14. Milk in losses: 162.5 units per game at $93.8 vs their 181.9 at $102.2,
they sell 52% at dawn (we 26%); in our 50 wins our milk price is higher ($117.9 vs $107.8).

## Why the melon fix did not show a big local gain (user question 19:30)

Two different changes were tested:
1. Early melon selling inside the anticipation stack: positive (Local-LB +3.0k, with collectall
   +4.1k [+2.1k, +6.0k], all five opponents positive). But it does not win the melon race yet:
   median first melon sale on day 10 moved from hour 23 to hour 11, while the Local-LB family
   (the Kaggle public plan) sells from hour 9; we sell melons first in 0% of Local-LB games under
   every variant. Our melon price rose only from $171 to $184 against their $225-234, so the
   stack's gain comes mostly from milk and wool, not melons.
2. More melons (the opponents' opening, 2 sheep + 12 melons on day 0): negative, because the $960
   of melon seed pays back on day 10, after the herd-buying days 6-9 (dawn cash $387 vs $506 on day
   7, $1,237 vs $1,770 on day 10; herd 9.6 vs 11.1 animals on day 7), and the extra melons (74 units)
   sell second at $167; more melons also slowed the routes (first sale hour 18).
What was missing: being first. The melons must be in the shed by hour 8 on day 10; our routes
reach hour 10-11. The fix is a routing routine for ripe-melon days (melons on the tiles nearest
the shed, or the farmer ending day 9 beside the melons so the harvest-and-return trip starts at
hour 0), not a sale rule or a larger count.
