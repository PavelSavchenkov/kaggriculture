# 24-hour gap cycles (Sep 25 19:45 - Sep 26 19:45)

User request (Sep 25 19:45): every 3 hours, look at all available data (Kaggle games of our
submitted agent, local league/zoo, self-play/mirror, Local-LB), pick the top 3 conceptual gaps in
the pipeline (BC or day compiler), iterate on them for 3 hours (switch to the most underexplored
or highest-ROI gap), document for other sessions. Fast iterations and cheap checks first,
conceptual and simple ideas, no monkey patches.

Test ladder (cheapest first): 8 traced mirror games (`mirror_small.sh`, ~35 s) -> 31 frozen
Kaggle games (`frozen*.sh` subset) -> 160-game Local-LB (`run_exp*.sh`, ~8 min, confirmation only).
Probe code: `exp_patch/` (env flags, defaults unchanged), one build dir per probe set
(`build_patchN`). Earlier material: `GAPS.md` (status table at the top), `ROTATION_LOG.md`,
`KAGGLE_REPLAYS.md`.

## Cycle 1 (Sep 25 19:50 - 22:45)

Data reviewed: 64 Kaggle games of pavel-bc-opus-v12-robust (50 wins, 14 losses to ranks 5-49;
`q58.py`), 160 Local-LB games per variant (x13, anticipation, collectall, their combination),
224 zoo games, the X13 mirror bed, 145 frozen top-10 replays.

Top 3 gaps:

1. **Time-critical delivery (compiler).** Races for high-value products are decided by who sells
   first after the price is high: melons (no shop demand; the opponent sold first in 12 of 14
   losses, melon -5.2k mean) and dawn milk/wool (milk -3.3k in 13 of 14 losses; strong players
   sell 52% of milk at dawn, we 26%). The compiler plans deliveries by a fixed deadline and hires
   that act from hour 1, so our melons reach the shed at hour 10-11 at best. Conceptual fix:
   delivery hour as a planned decision valued by an opponent-anticipating market model, the
   farmer's overnight position as a resource for tomorrow's first job, and dawn sales of held
   stock ahead of the opponent.
2. **Fertilizer economy (BC + compiler).** Fertilizer revenue -3.0k in 12 of 14 losses and -5.4k
   per game vs the Local-LB family. Part is missed collections (fixed by collect-all: +1.1k to
   +2.3k), part is that we put ~200 units on crops while the opponents sell theirs. Question: does
   our fertilizing pay at the margin, and should the compiler value a fertilize job against the
   sale price?
3. **Labour cost (compiler).** Wages -1.9k in 12 of 14 losses. Core = walking (+13 moves/day);
   wage-aware returns +0.5k; wider search not robust. Open: deposit timing and trip structure
   (use the night auto-deposit instead of mid-day returns).

Log (newest last):

- 19:55 (gap 1) Farmer pre-positioning for the melon race (`OPUS_FARMER_MELON`: an idle farmer walks
  onto tomorrow's ripening melon in the evening): no effect, 8/8 screen games identical to the
  dollar. Debug: the farmer does walk (d9 h20-23), but the engine resets the farmer to the NW
  shed tile at every day end (`fast_game_engine/sim.hpp` `end_of_day`: `pos_x[0] = pos_y[0] =
  board/2 - 1`; not stated in `prompts/game_rules.md`). Nobody can pre-position overnight. The
  physical limit for melons ~4 tiles from the shed: the farmer (acts from hour 0) harvests at
  hour ~4-5 and deposits at hour ~8-9; hires act from hour 1. Opponents sell from hour 9, i.e.
  at that limit; the anticipation stack reaches hour 10. Remaining melon-race potential: about one
  hour (make the farmer's first job the melon trip), or melons planted nearer the shed (the tiles
  next to the shed are soft-reserved for animals, which need daily visits). Closed as a big lever.

- 20:10 (gap 2) Fertilizer economy is mostly not a gap. Rules: a fertilizer on a one-shot crop
  makes each eligible watering add 2 instead of 1 (wheat: +2 units, ~$70-90), on an ongoing crop
  +1 unit per production (strawberry $100+); it sells for $55-65 and, like melons, has no shop
  demand, so its price only falls as it is sold. Our "fertilizer revenue gap" (-3.0k in Kaggle
  losses, -5.4k vs the Local-LB family) is composition: we turn ~200 units into crops, opponents
  sell theirs. In the 14 Kaggle losses fertilizer + strawberries + crops net +3.7k for us. The real
  loss is missed collections (fixed by collect-all). Closed; gap 2 slot freed.

- 20:25 (gap 3) Route audit (`tools/route_audit.cpp`, per worker-day: moves vs the Manhattan MST over
  its start and work tiles; days 10-28; 64 Kaggle + 160 Local-LB of ours, 300 top-team games):

  | | workers | moves | MST | moves/MST | MST per tile | tiles per worker |
  |---|---:|---:|---:|---:|---:|---:|
  | ours (Kaggle, Local-LB) | 12.6 | 123 | 103 | 1.19 | 1.49-1.50 | 5.5 |
  | top-10 | 12.1 | 110 | 90 | 1.22 | 1.27 | 5.9 |

  Routing is not the problem: given its jobs, each of our workers walks as close to the lower
  bound as top teams' (1.19 vs 1.22). The extra walking is in the job sets: each worker's tiles
  are 18% more spread (MST per tile 1.50 vs 1.27), with more workers holding fewer tiles each (every
  worker starts at the shed, so each extra worker pays its own walk out). Probe `OPUS_ROUTE_QUAD`
  (weight of the load-balancing time^2 term in route values; 8-game screens): 25% -> MST/tile
  1.49, wages $346 -> $331/day, margin -105; 0% -> 1.51, $330/day, +730. The quadratic term is not
  the cause. Next idea: the min-hire search with deadlines produces many short job lists; a
  "fewer, fuller routes" objective (compare K and K-1 workers by walking, not only feasibility).

- 20:35 New data source: the 11 frozen top-10 games we lose (`ftr/base_lost`, re-run with traces,
  1 min). Ours minus the top team: milk -7.8k (10 of 11), melons -1.2k (9 of 11), strawberries
  +2.9k, crops +2.5k, wages +1.9k in our favour (fertilizer column is a frozen-replay artifact:
  Boey's recorded fertilizer buy/sell orders run against a different market). Milk = herd size:
  top teams 10.2 cows vs our 7.8 (milk per cow-day 1.13 vs 1.09, price $100 vs $90).
- 20:40 Gap 2 slot -> **herd size at the investment days (BC + budget)**. Top-team herds at day 12
  (1,506 perspectives): DSM 20.9 animals (9.1 cows; 91% wins in this data), Vadim 20.8, DECEM 20.7,
  Mother-Goose 19.6; ours 17.2 (7.6 cows), with Boey, Majkel, Kaggledew the smallest. Own final
  money per animal at day 12 (top-10 regression, `q59.py`): cow +2,121 (se 233), sheep +875 (181),
  goose -302 (213). Probe `OPUS_MID_STYLE=7:6:14` (DSM style for days 6-14): herd 17.0 -> 17.2,
  cows 8.8 -> 9.5, margin +407 (8 games); days 6-20: -384. The budget binds, not the style.
- 20:45 Where the strong teams' herd money comes from (days 0-12, per game): revenue +2.0k (melons
  +2.5k: $13.3k vs our $10.8k; everything else equal), spent on animals (+$1.4-1.6k: $8.6-8.8k vs
  $7.2k) and land. Melon money arrives on days 10-11, exactly when the herd is expanded. So the
  melon race (gap 1) and the herd gap (gap 2) are one chain: selling first melons late/cheap costs
  ~$2.5k at the investment moment, which becomes ~3 fewer animals and the milk/egg deficits
  seen in Kaggle and frozen losses for the rest of the game.

- 21:05 (gap 1, user: "why are we struggling with melons, it should be easy") Three compiler details,
  none of them the network: (1) placement: animals get the tiles next to the shed first, melons
  land ~3.9 tiles away (same as opponents); (2) delivery: the race deadline asked for all melons at
  one early hour, which is unroutable (one worker harvests ~1 tile per 2 turns), so the ladder fell
  back to the evening deposit; (3) sale: no shop demand + no opponent melon history = flat DP, ties
  go to the latest hour (fixed by melon tie-now). Strong players stream: one tile (6 units) per hour
  from hour ~9, the farmer (acting from hour 0) bringing the first.
  Probes on all 64 frozen Kaggle games (opponent's recorded melon sales), on top of the anticipation
  stack: `OPUS_MELON_FIRST=6:3` (first tile by the early hour, the rest 3 h later): first melon sale
  median hour 10 -> 8, we sell first in 17% -> 52% of games, margin +85 (one tile first is worth
  little). `OPUS_MELON_STREAM=6:6` (cumulative target 6 + 6 per hour): +883 [-78, +1,951], own
  +1,098, better in 37/64; our melon price $198 -> $202 vs the opponents' $193: price parity
  reached. What is left of the melon gap is the count (65 vs 80 units: 10 vs ~13 plants), which the
  opening budget limits (the 2-sheep/12-melon opening loses, -4.6k). Local-LB confirmation running
  (`lb13_ops` vs `lb13_ops_ms`, same build).
