# Crop conversion audit checkpoint

Status: evidence and reusable service templates complete; proposals are **uncompiled and unpromoted**. This checkpoint does not establish a stronger agent.

Compared 36 player-games from six globally selected players in `refresh_1212` and `refresh_1112` with 64 games of `investment_context_guarded_001_best` against `public_router` (seeds 1000–1031, both seats). Eight local games also have exact C++ event traces; their cash, action hashes, crop output and fertilizer counts match the original profiles. Selection metadata, episode/seat/submission IDs and raw replay hashes are preserved in the JSON artifacts.

## Main finding

The fertilizer gap is mainly within wheat and carrot. The local reference has no productive fertilizer coverage for those crops, while its strawberry coverage is already 90.85%. Its nominal productive coverage across crops is 19.70%; crop mix alone does not explain the large gaps versus Atakan, Mengfei, Crop Dusta or 自己找差距. See `mix_decomposition.csv` for the exact within-crop and mix terms. Coverage counts eligible dated water/production opportunities; it does not prove marginal yield after caps or marginal profit.

| Crop | Local harvested units per seed | Local units per occupied crop-day | Relevant donor evidence |
|---|---:|---:|---|
| Wheat | 3.166 | 0.756 | Mengfei 4.309 per seed; Atakan 4.156 |
| Carrot | 2.645 | 0.701 | Atakan 3.551 per seed; Mengfei 3.410 |
| Strawberry | 7.547 | 0.432 | Atakan 7.896; 3정훈 has 97.14% productive fertilizer coverage |
| Melon | 6.000 | 0.545 | Local reaches the stored-yield cap without fertilizer |
| Tomato | Absent | Absent | Mengfei averages 12.17 plants and 97.33 harvested tomatoes per game; 8 per seed |

Full observed farm cash and fertilizer material balances reconcile in all 36 donor games. Production, product purchases and product sales are separate in `full_observed_economics.json`, `transactions.csv` and `fertilizer_economics.csv`; bought-and-resold stock is not counted as crop production. Animals supply most fertilizer, but using it still forgoes its sale value. Of 5,057 donor fertilizer applications, none merely duplicated the existing expiry, and 17 had no subsequent eligible productive water/update under the observed continuation (16 Crop Dusta, one ymg_aq). This is a conservative certificate; other cap or overlap waste may remain.

## Reusable proposals

Priority combines ease of compilation with missing strategic coverage. The quoted dollar deltas are **shadow values at the donor's contemporaneous harvest and fertilizer quotes**, before labor, land, funding, changed market prices and rival revenue. They are not realized marginal profit.

1. `strawberry_day20_completion`: 3정훈, episode 106446230, submission 56063995, cell (9,3), planted day 7. Fertilize days 16 and 20; water on the production-support days; harvest two berries on days 17, 19, 21 and 23. Local matched cell harvests six, donor eight. One extra fertilizer; shadow delta +$102. There are 128 matching two-fertilizer/eight-output instances across six donor games. The donor exit also contains a crop DIG, so its full tile course is not literally a single inserted action.
2. `wheat_day14_conversion`: ymg_aq, episode 106443319, submission 56065395, cell (3,0), planted day 12. Fertilize at step 349/day 14, harvest six at step 400/day 16 versus four locally. One extra fertilizer and one additional WATER event; shadow delta −$24. The 114 repeated examples establish a service pattern, not an unconditional economic improvement.
3. `tomato_rotation_after_day9`: Mengfei Li, episode 106429645, seat 0, submission 56047440, cell (0,1). Wheat day 9→13, tomato day 13→25, final carrot day 26→29. Tomato WATER days 13,15,17,19,20,21,22,23; FERTILIZE steps 500/day 20 and 566/day 23; four two-unit harvests at steps 527,547,568,592 on days 21–24. Relative to the matched local suffix: −13 wheat, +8 tomatoes, +1 carrot, +$10 seed cost, +2 fertilizer, three fewer PLANT and five fewer WATER events, equal HARVEST count. Shadow delta −$53 before possible labor savings. Weed clearing after tomato expiry is in the full tile events, outside crop-only operation counts. There are 73 matching tomato lifecycles across six Mengfei games. This is a useful larger family proposal because the current reference has no tomatoes; current evidence does not justify unconditional replacement.
4. `carrot_day28_conversion`: ymg_aq, episode 106443319, cell (8,3), planted day 26. Fertilize step 680/day 28, harvest four step 702/day 29 versus three locally. One extra fertilizer; shadow delta +$14. There are 98 matching examples across five games.

Each proposal folder contains the donor tile events, complete donor day actions, local and donor lifecycles, exact input/output accounting, source hashes and implementation dependencies. Donor full-day actions require recompilation because surrounding farms and inventories differ. The tomato suffix also needs replacement of lost wheat before animal feed is due, tomato sale orders, weed clearing, and the final carrot schedule. Every edit must rebuild fertilizer reserves/pickup, worker days, dated deposits/sales, funding, and downstream physical guards. Root's separate fertilization work found that a biologically successful edit can disable later worker schedules and lose more labor cash than its extra crop earns.

## Reproduction and scope

Run from the repository root, in this order:

```sh
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/analyze.py
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/enrich.py
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/local_events.py
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/proposals.py
```

These are offline extraction/formatting scripts plus a C++ exact trace driver, not Python gameplay agents. `LOCAL_TRACE_VALIDATION.json` retains the exact build and invocation, frozen dependency manifest and binary hash. `SOURCE_HASHES.json`, `REPLAY_HASHES.json`, and per-proposal hashes identify inputs. Original machine paths in historical command records require rebasing after relocation.

Public replay behavior establishes observed actions and outcomes, not private donor implementation or intent. Correct public player name: **Atakan Aldemir**. An earlier frozen Atakan portfolio README incorrectly says “Atakan Mamedov”; its episode IDs and hashes remain authoritative.
