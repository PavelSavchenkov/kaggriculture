# Fresh global sources at 14:46 UTC

The current top twelve teams supply 72 selected player-games and 58 unique replays. The established exact extractor reconciles all selected cash outcomes. This cohort contains 57,763 transactions, 16,334 crop lives and 1,186 animal lives. Source IDs, dates and scores are in top_replay_manifest.csv; retrieval commands and raw replay hashes are in REFRESH.json.

Rank order: 3정훈, Mengfei Li, SpaTaro, get some fries, ymg_aq, 自己找差距, Suliman Tadros, Jiro2, Atakan Aldemir, binghua, Crop Dusta, Bohann Wang.

The full eighty-metric comparison with the local crop_value_m2_t4 reference is in reference_comparison.json and research/review_40.md. Different opponents and shop histories prevent treating the aggregate cash difference as causal. The local farm remains cheaper in labor and lower in faults/discards, while crop-specific conversion and composition remain improvement targets.

New concrete donor examples are in larger_crop_examples.json, with complete day actions, exact service dates, seats and replay hashes:

- Get some fries, episode 106458209, cell (3,2): WHEAT0→4 yields4, MELON4→14 yields6, WHEAT14→18 yields6, WHEAT18→22 yields6, WHEAT22→26 yields6, CARROT26→29 yields3.
- Same episode, cell (4,0): CARROT0→3 yields3, MELON3→13 yields6, TOMATO13→24 yields8, CARROT24→27 yields3. It clears and reuses the tomato tile on the final harvest day; these timings are evidence for earlier tile reuse, not a tested local profit.
- 3정훈, episode 106455464, cell (3,2): MELON0→10 yields6, TOMATO13→24 yields8, WHEAT24→28 yields4. The gap before tomato planting is retained. Do not fill it without supply and route checks.

The fresh animal-decision extractor finds 951 animal purchase orders, 1,186 placements and 124 comparable purchase-pair candidates. Those candidates need state-by-state review before attributing a branch to shops or opponents; no new animal rule is promoted from this refresh.

Only destbreso/x-ray-your-agent has a changed notebook timestamp, 14:29 UTC. Its 66,982 bytes of code are identical to the 05:04 copy. It is an analysis notebook, not a new gameplay policy, so no port is needed. The raw notebook, metadata, extracted source, empty code diff and SHA-256 audit are retained in notebooks/x_ray/. Its early-shop grouping and action-similarity methods are diagnostic heuristics; similarity is not proof of private implementation or causal adaptivity.

All extraction is offline Python. Gameplay policy remains C++. No submission was made by this refresh.
