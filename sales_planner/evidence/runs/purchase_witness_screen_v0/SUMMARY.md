# Fixed purchase quantities from a funded schedule

The unchanged delivery-bound sale rule gains $255.91 mean full-game margin over original orders on 394 supplied player plans from 197 exposed episodes (episode-cluster 95% interval $210.44–304.31). There are 336 gains, no losses and 58 unchanged cases. All checked physical state, production, terminal resources, faults and shops match; all own cash predictions match realized gains.

This is a narrower input contract. The supplied schedule fixes each purchase quantity to the amount that actually executed in its source game. Purchase dates and order positions remain fixed; sales can move. Future own purchase quantities are provided plan data, not a forecast inferred from live observations. Rival future actions and private stock do not enter the sale rule. This is not a general purchase optimizer.

Normalization alone changes 8,716 requested orders but preserves both players' cash, physical state, shed, production, discards, shared market inventory and shops at all 283,286 checked turns. See NORMALIZATION_SUMMARY.json and NORMALIZATION_CASES.jsonl. The complete normalization check takes 1.01 seconds.

Against unnormalized delivery timing, four cases improve margin and 390 match: +$0.85 mean margin ($0.03–2.38). All three prior state exceptions disappear. The small average difference is not the main result; the useful change is preventing extra cash from activating purchases outside the supplied plan. Own cash is $3.53 lower on average than the unnormalized overlay because that overlay changed farming and helped the rival in one case. Keep this comparison separate from the +$255.91 over original schedules.

CHANGED_CASES.json keeps all four differences. The failed future-restock shortcut remains rejected in runs/seed_restock_screen_v0. Fresh public confirmation is next; these 197 episodes were already exposed, and no improved overall plan selection is established.
