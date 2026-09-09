# Complete tomato rotations: current evidence

The Mengfei-derived tomato block compiles into complete day 12–28 courses, with
every day checked against the requested tile, shed and seed endpoint in a full
game. This establishes that the compiler can realize a crop-family replacement.
The two compiled variants are weaker than their source on the discovery panel.

Source: `investment_context_guarded_001_best`, now submitted as 56078898.
Donor: Mengfei Li, episode 106429645, seat 0, cell (0,1), submission 56047440.
The donor's planting, watering, fertilizer and harvest dates and source hashes
are retained in `research/crop_conversion_audit_001/` and each run's `RUN.json`.
The implementation retains surrounding crop and animal obligations, replaces
lost feed with fewer wheat sales or purchases, and sells the added tomatoes.

Both screens use the same 32 discovery seeds, 1000–1031, in both seats against
`public_router`. The physical entry conditions match in 46 of 64 games. These
are diagnosis data, not an independent promotion panel.

| Change | One tile, cell 10 | Three tiles, cells 22/31/40 |
| --- | ---: | ---: |
| Output per activated game | −14 wheat, +8 tomato | −36 wheat, −3 carrot, +24 tomato |
| Mean own cash change, all 64 games | −$293.23 | −$378.20 |
| Mean opponent cash change | +$73.31 | +$226.55 |
| Mean extra labor cost | $295.41 | $398.91 |
| Own cash change before extra labor | +$2.17 | +$20.70 |
| Wins | 54/64 | 54/64 |
| Source wins | 57/64 | 57/64 |

The replacement's own cash before labor is nearly flat on this panel. The
current schedules add four hires per activated one-tile game and five per
activated three-tile game. Price changes also help the opponent. More tomatoes
alone therefore do not establish a stronger composition under this scenario.
The result does not prove that the composition cannot work with cheaper routes,
different tiles or different observed demand.

The three-tile change also delays the last carrot planting from day 25 to day
26, losing one carrot per tile. The current cheap estimate omits this partial
continuation and can omit the retained wheat harvest before the tomato planting.
It also values avoided seed use as avoided expenditure, while compilation still
retains the old seed purchases. Those errors must be fixed before using its
ranking for broader rotation search. Labor, endogenous prices and opponent
response remain explicitly excluded from that preliminary fixed-quote estimate.

Next work:

1. Estimate the full changed crop lifecycles, including the retained first
   harvest and shifted last carrot, and compare each dated output with these
   exact realizations.
2. Cancel unused seed purchases when their timing and cash dependencies permit.
3. Compare route reuse, neighboring tile blocks and alternative productive dates
   before accepting extra workers. Keep unresolved compiler cost separate from
   an economic rejection of the requested composition.
4. Evaluate observed-shop branches with full own and public-opponent crop flows.
   The existing unconditional variants remain unpromoted.

Evidence: `runs/crop_rotation_002/`, `runs/crop_rotation_003/`, their frozen source
snapshots, all day contracts and exact paired games. Reproduce the profile
comparison with `scripts/summarize_crop_rotation.py crop_rotation_003` through
the `kaggriculture` conda environment.
