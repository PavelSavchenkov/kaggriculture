These exposed contracts contain no replay paths. Each main cold regression uses a 12-second, single-thread budget and strict replay.

- `early_milk_handoff.json`: UnknownMother 108286602, seat 1, day 8, original 11 workers. Requires 12 milk by hour 3. A feed/harvest pair needs a same-hour handoff; whole and two-action jobs missed 6 milk.
- `staged_seed_and_animal_purchases.json`: ymg 108270468, seat 1, day 6, original 9 workers plus one. Seed and animal purchases occur throughout the day. Match their quantities to the latest possible planting and pickup times when ranking routes.
- `unpolished_completion.json`: UnknownMother 108286602, seat 1, day 18, original 12 workers plus three. The unpolished route completes within the beam budget while the lower-scoring polished route times out.
- `third_day14.json`: THIRD FARM CLUB 108286685, seat 1, day 14, original 12 workers. Purchase-aware route scoring recovers a prior original-worker regression.
- `regret_third_day15.json`: THIRD FARM CLUB 108281701, seat 0, day 15, original 12 workers. Regret insertion finds a route partition missed by the fixed job order. No replay routes or task times are included.
- `rebuilt_unknown_day25.json`: UnknownMother 108286602, seat 1, day 25, original 12 workers. Partial reconstruction must preserve the retained inventory and the required 17 units discarded at night.
- `early_fertilizer_late_wheat.json`: UnknownMother 108276124, seat 1, day 1, original 5 workers minus one. Initial wheat must serve animals whose fertilizer is sold early; reserve later wheat for an animal without that early deadline. The previous constructor assigned late wheat to an early job and its partial-route fallback threw while rebuilding the same invalid model.

Release1.2 adds two cold search controls. `local_wheat_sale.json` is M&M
108293743, seat0, day2 at the five original workers. `idle_last_hire.json` is
Majkel108264640, seat0, day15 at original+3 workers. Both require exact
outputs and retained inventory; neither includes source routes. The search
regression group tests the former with RegretFast at500ms and the latter
with the four-second portfolio, including restoration of an idle final hire.
