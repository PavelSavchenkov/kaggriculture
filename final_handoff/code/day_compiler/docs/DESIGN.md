# dc12: an M&M-like day compiler (design, Sep 29 18:20)

dc12 is designed from M&M's measured behaviour and the engine's market rules, not by patching dc11. User (18:20): even
"trusted" components are re-read in full for hidden assumptions before anything is reused. Audit split (each writes
assumption / lines / evidence vs M&M / severity to its findings file): bind (intent -> stops): BC; funding (funded,
variants, trims, fallback levels): Weaknesses; market + executor seller: Imitation; router + realize + executor shell + the
integration: Day compiler. The candidates for reuse, pending that audit:
- the engine simulation;
- the router and search (crew at M&M's size with dropany, no waits, no supply re-picks);
- bind / realize;
- the funding simulation, which runs whatever seller it is given.

The market model, the seller and the deposit values are rebuilt: that is where dc11's assumptions differ from M&M's
(REWORK.md).

## Engine facts the design rests on

- Market inventory is linear. Each sale adds 1 permanently ($1 sales add nothing). Drains subtract fixed amounts after
  each hour's sales: shops every 4 hours at h0 / h4 / ... / h20 (a shop drains the products it buys, `mult` each), and the
  center 1 unit of each non-fertilizer product at h0. So h1, h5, h9, h13, h17 and h21 are the first hours after a drain.
- Price is a fixed curve of inventory, so a unit's price is set by all sales before it minus all drains before it.
- Within an hour, orders run slot by slot for both players (slot 0 of both, then slot 1, ...). Units in the same slot
  interleave at shared quotes; an earlier slot executes fully first. "First" means an earlier hour or an earlier slot.

## Why dc11 sells late, and why M&M wins

- dc11's DP treats the opponent's supply as a fixed forecast flow. Against a waiting opponent, waiting is right: one lot
  just before the opponent's evening dump captures all the day's accumulated drains, at better prices than window by window.
- The mirror is a game of chicken. Both sides wait and dump into one crowded evening (evening milk 80 units vs 54 in M&M's
  worlds). A side that takes each drain as it comes leaves the waiter an empty market. That is M&M against our lineage: its
  own revenue equals our copy's, while the opponent loses 2.8k.
- M&M's pattern (steady slices at post-drain hours, bigger when the price is well away from its trailing level, dawn-first
  after the overnight drain, a stock buffer) is the policy of a seller that expects a competitor to take any drain it leaves.

## dc12 seller (the core)

The opponent is modelled as a competitor: any drain we leave is taken before our next chance. So the value of waiting past a
drain window is the price at the current inventory, not an accumulated-drain price. Consequences, each derived rather than
scheduled:
1. At each post-drain hour, sell first (early order slots) the units the drain freed: up to the drain amount, while the
   quote beats the value of holding.
2. Sell more than that only when the quote beats the expected future level. That level is the price at the inventory the
   market returns to when both sides take drains, i.e. near the current level; units above it are better held for later
   windows, which keeps a buffer.
3. Hold overnight what the next dawn window can absorb first (the overnight drains, h20 + h0). Sell the rest before the night
   only as the shed room forces.
4. Deposit values come from this seller. A unit is worth more if it reaches the shed before a window where our stock is
   short of the drain, so routes deliver output before windows (morning service follows from value, not from a time block).
5. The funding simulation runs this seller. Morning slices bring cash in the morning, so purchases such as the day-3 cow at
   h8 can be funded from same-day sales (M&M's cash path) without a special funding rule.

Products: v0 applies the race seller to strawberries, milk and wool (the thin, contested books). Eggs (deep market, M&M sells
them late), melons (whole stock at dawn / morning), wheat and fertilizer (feed / fertilize reserves) keep dc11's handling
until measured.

## Milestones and judges

- M1 seller v0: the rules above for strawberries, milk and wool. Judges: 6 pinned M&M worlds in both seats (arm = copy with
  dc12; sub = our package with dc12), then Imitation's 48-world M&M bed and the local league.
- M2 deposit values from the seller (rule 4): check days 12-17 on M&M's intents move animal service to the morning by value.
- M3 price conditioning: trailing inventory level (more when far from it, as M&M does).
- M4 other products (eggs, melons, tomatoes).
- M5 funding: verify the day-3 cow and days 3-8 investment follow from the morning cash path.
Every milestone keeps dc11's validated parts. Default play of dc11 is untouched: dc12 is a separate build.

## Synthesis after the full audit (18:45; AUDIT.md + findings/*.md)

Reused as is: engine sim, router / search (R1-R12 hold; dropany on), realize, the executor shell. Rebuilt, each as a model:

1. Seller (core; audit M1, M2, M5, M9). The opponent is a reservation-level seller: when drains push the market inventory
   below its level L_p, it sells back up to L_p while it has stock A_p (inferred shed stock + collectible visible output);
   above L_p it holds. L_p is the inventory level the market kept over the last day (history of observed inventories).
   Under that response the price cannot stay above price(L_p) while the opponent has stock, so:
   - at a post-drain hour, sell first (early slots) the units that bring the inventory back to L_p;
   - hold the rest while our stock plus today's incoming output fits the drains to come through tomorrow's first
     post-drain hour; sell the excess now, when it cannot fit;
   - if the opponent has no stock of p, nobody fills the drains and dc11's DP (drains accumulate for us) is the right model.
   The rule is one consistent plan re-evaluated each hour (fixes the time inconsistency M2). The deposit values and the
   hold value come from the same model (M5, M9).
2. Funding (Weaknesses' audit 1-5). Purchases are scheduled on the cash path of the plan: the funding simulation orders each
   purchase at the first hour its cash is there, counting sellable shed stock (fertilizer and wheat above the reserves)
   as cash. The routes then place and plant after those hours. Animals are not ranked last; trims follow the network's
   own drop order (drop_loss) instead of a fixed crop / animal order.
3. Values (BC's bind audit 1, 3, 6). Service values follow production: feed + care bank one unit of milk / wool / eggs
   (its price) on production days. Ongoing-crop waterings are valued by their yield effect, not only when dry. Water-only
   one-shot stops are valued by the marginal yield, not the whole plant.

Build order: 1 (seller + L_p history + deposit values), then 2, then 3; each judged in both seats on the duel beds, then
the league.

## Revision after the isolation ladder (20:25; README "Gate ladder", PROGRESS 19:35-20:25)

What the ladder measured on M&M's own recorded states and intents, against the recorded opponent:
- The dc11 execution core (bind, router, realize, executor) reproduces M&M's day on days 12-17: the same plants per crop,
  harvests, field units (87.6 vs 87.5) and field value (within $5), and a better one-day margin (+90 / day). Rebuilding the core
  from scratch is not supported by evidence; its measured faults are in the couplings between components.
- The seller rules of the synthesis above (reservation-level opponent) are closed: they lost in every judge (stop list).
The couplings, each traced to its root cause and fixed as a model change:
1. Order slots vs crew (fixed, saleslots=3). The engine takes 10 orders a turn, one per hire; the router filled the first hour
   with hires and the seller's dawn lots were dropped. The dawn decision now takes a hire's first-hour slot only when routing
   again with the smaller first wave keeps the crew and drops no required stop. Pinned real games +362 (SE 109).
2. Selling vs funding (partly fixed: cashsell = the seller's Lagrangian cash constraint, wheatcash, reserve=0). M&M sells each
   morning's collections by midday, so its purchases are funded at h5-h9; our seller and router hold for the evening, so the
   funding check defers or trims the day's investments (day 3 cow, day 6 wave, day 10 land). Open: the router's deposit values
   come from the evening-selling DP and ignore the cash need (collected fertilizer carried until ~h9); the cash constraint's
   shadow price should enter the deposit values as it enters the seller.
3. Forecast (BC's area). The true opponent flow is worth +86 / day on rung 1 (volume and timing about equal; milk first). BC's
   fresh forecaster +25 / day on 216 held-out M&M games.
4. The evening bias vs reacting opponents (open; rung 3). One day against a fixed opponent, holding for the evening wins; in the
   mirror and live, later lots realize $3-8 below prediction (Imitation M1). No local bed has opponents that react like the top
   teams; judged on contested beds and live only.
