# dc12 rework: dc11's design assumptions against M&M (Sep 29 evening)

Goal (user): rework the day compiler from its design assumptions so the agent plays as well as M&M or better. Do not tune
switches on a black box: find which of our design ideas are wrong and rebuild those parts.

Judges, in order: (1) same-world duels (Imitation's runs/duel/mm.sh: 48 M&M worlds vs live d3crop, and the 6 pinned worlds);
(2) the local league (copy and d3crop vs our 8 real agents); (3) Weaknesses' 760 + wide bed; (4) live. Recorded-opponent beds
(pinned replays, teacher_day continuations) cannot show denial and read seller changes with the wrong sign.

## What M&M does that we do not (measured)

- Sells thin products first, at dawn and in the morning, at the day's best prices (milk h0-2: 34 units at $126, 72 M&M
  worlds). It leaves the evening to the opponent; our lineage holds for the afternoon / evening, where both farms compete.
- Serves animals in the morning (h3-8: collect 11.3, care 9.1, feed 9.0 per day), so the output is ready for the next
  dawn / morning sales. Our routes do ~2 fewer of each there and make them up at h15-23 (days 12-17, M&M's intents).
- Sells per lot on price: more when the quote is well above or well below its trailing mean, least near it. A fixed hourly
  share schedule loses (-6.2k on 47 worlds): the dawn-first pattern works because of what M&M holds at dawn and its price
  test, not the shares.
- Funds its day from same-day early sales (day-3 cow at h8 from fertilizer sold all morning; dawns 3-5 nearly broke). We end
  those days with $140-250 idle and without the cow.
- Its own revenue equals our copy's; its edge is the opponent earning less (+2.8k for d3crop vs the copy than vs M&M).

## dc11's assumptions, the evidence, and the rebuild

1. Sale DP market model: inventory = deterministic shop drain (every 4 hours) - a point forecast of the opponent's hourly
   sales. Under a drain model waiting always raises the expected price, so the DP sells as late as the forecast allows.
   Evidence: our evening share 31-50% vs M&M's 8-18%; M&M's dawn prices are the day's highest. Rebuild: a market model in
   which the first seller after a drain takes the drained price and later sellers share a depressed market (queue order
   matters), with the opponent's supply treated as a response, not a fixed flow.
2. Held stock is valued at tomorrow-noon prices x 0.95 after the opponent's morning supply (hold_supply), i.e. we assume we
   lose the dawn race. Rebuild: value held units at tomorrow's first post-drain hour, sold first (dawn-first hold value), with
   our own next-morning production counted.
3. Deposit values (the router's value of a collected unit by hour) come from the same DP. If the DP sells in the evening,
   an early deposit is worth no more than a late one, so routes collect animal output in the afternoon. Rebuild: deposit
   values from the reworked seller; morning service then follows from value, not from a time block (the morning block alone:
   -3.0k in duels, because the output was still sold the same evening).
4. Rival weight counts only the opponent's forecast units within today's hours. Denial through inventory that persists into
   the opponent's selling hours (tonight / tomorrow) is not valued. Rebuild: count the opponent's forecast sales over the
   hold horizon too.
5. Funding: purchases are fixed to hours; the stress forecast decides; animals are all-or-nothing and get trimmed. Early cash
   from same-day sales is not planned (day 3: cow asked 93%, bought 4% in our games). Rebuild: the day plan schedules sales
   that fund the day's purchases (sales as part of the plan, not only the executor's reaction).
6. Router: correct and efficient (dropany closes the crew gap; no waits; no supply re-picks). It follows deposit values, so
   it changes with 3.

## Order of work

1. Dawn-first hold value (2) as key `dawnfirst`: judge on the 6 + 48 M&M worlds (arm = copy in M&M's seat; and our sub).
2. Deposit values from the dawn-first seller (3): check that routes move animal service to the morning by value.
3. Denial over the hold horizon (4).
4. Queue-aware market model (1): the larger rebuild, if 1-3 show the direction.
5. Sale-funded purchases (5).
7. Yield waterings of one-shot crops (Imitation, 72 worlds): M&M waters melons at ages 6-12 and tomatoes d8-16 more fully
   (melon waterings -10 per game for the copy, each +1 yield: ~-$2.1k melon sales; tomatoes -$1.0k). On M&M's own intents
   our days 12-17 water -6 per day. Check whether the bind drops or undervalues intended yield waterings.

## Philosophy (for every session)

The compiler's choices come from explicit models: the market model and hold value (seller), deposit values (routes), the
funding simulation (purchases). When our agent differs from M&M, find the model assumption that produces the difference and
fix that model. Do not add switches, time blocks, multipliers, schedule tables or decode pushes that override a model's
output: each one tested so far lost, because the rest of the system still acts on the wrong model (the morning block made
the routes collect early, and the seller then sold that output the same evening).

## Proven not promising (do not spend tokens on these)

- Time blocks / schedules overriding the models: morning animal block alone (duels -3.0k), + hold 1.0 (-3.3k), + hold 1.1
  (-19.4k); M&M hourly-share sale table (mmpolicy) on the copy (-6.2k paired, 47 worlds; -2.3k on 6); regime M (-4.0k pinned,
  440 games); dayhold (-0.2k); dawnwait (-1.2k); racing after shop ticks (-4.8k); lot caps on thin products (lotcapthin -1.04k
  wide bed).
- Hold-value multipliers and seller terms bolted on the current model: hold 0.85-1.1, deny (-0.94k pinned, p 0.0002),
  dawnfirst on the current market model (no behaviour change: the evening dump stays 33 vs 35 units/day), rival weight 1.5
  (-0.37k).
- Funding patches: late crew, feed-cash guard, soft animal purchases (loses both cows), nightbuy (cows +0.2 but plantings
  -0.8 to -2.2), forced bigger early crews (day-3 cow unchanged), land financing for day-10 Q4, bigger compile budgets.
- Required daily watering (gate plantings -10); leaner routes via worker-turn value (-0.5k).
- Beds that cannot judge seller changes: pinned replays and teacher_day continuations (recorded opponents); reactive rule
  sellers for the replayed opponent (collapse it). Use the duel beds.

## Conceptual issues and who helps

1. Market model and hold value (items 1, 2, 4): Day compiler rebuilds the seller. Needed from Imitation: per hour band, the
   realized price of units sold by each side in the M&M worlds, and our DP's predicted price for the lots it chose (DC11_SELLLOG):
   the model error by hour, and whether being first after a drain changes the price a lot gets (queue order).
2. Sale-funded purchases on days 2-8 (item 5): Day compiler rebuilds funding. Needed from Weaknesses: M&M's cash path on days
   2-8 (hour of each purchase, cash before it, which sales funded it) from its recorded games, and the same for our lineage.
3. One-shot yield waterings (item 7): needed from BC: network water intents for melons / tomatoes at ages 6-12 vs the waterings
   dc11 executes (DC11_INTENTLOG), to tell the network's ask from the compiler's drops.
