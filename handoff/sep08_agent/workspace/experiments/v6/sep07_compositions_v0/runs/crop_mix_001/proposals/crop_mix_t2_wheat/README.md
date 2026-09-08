# crop_mix_t2_wheat

The current local search reference. At day 12, choose the complete three-tile
tomato replacement when at least two observed shops consume tomatoes and its
physical entry guard matches; otherwise use the productive one-tile wheat
course. Both retain the existing day-20 berry fertilizer branch.

Fresh evaluation covers 18 opponents with 1,024 games each. Four-group win
utility is 93.197%, versus 91.079% for the tomato parent. Additional direct
4,096-game results: 1,801W / 2,010T / 285L versus tomato; 874W / 2,800T / 422L
versus wheat. The broader league gain over wheat alone remains uncertain.

All 18,432 fresh full game records equal their selected component. Generic,
pair, debug, thread, native, PASS and self-play checks pass. Day-28 weed guards
can fall back to the parent while preserving production. See IMPORT.json for
component lineage and ../../../../results/crop_mix_validation.json for all
results and limitations. No Kaggle adapter or submission for this variant.
