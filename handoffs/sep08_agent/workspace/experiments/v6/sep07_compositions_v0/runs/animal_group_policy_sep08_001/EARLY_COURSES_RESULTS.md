# Early cow and goose courses, independently audited

Both courses complete719actual turns, every action is valid, and all30daily
endpoints pass an independent replay. Corrected land-event mapping unlocks the
previously failing day11. These remain fixed-world composition experiments.

| Course | Own gain | Margin gain | Extra labor | Cheap own | After actual labor | Residual |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| goose_pair_d10 | +519.00 | +595.00 | +432.00 | +1032.59 | +600.59 | -81.59 |
| cow_pair_d8 | -4145.00 | -857.00 | +1631.00 | -457.03 | -2088.03 | -2056.97 |

The goose pair advances two animals the source planned later; it adds19eggs and
5fertilizer, with unchanged crop output. This is not two permanently extra geese.
The cow pair adds48milk/40fertilizer, but loses16wheat/4carrots/6strawberries and
incurs much more labor. More production alone does not establish profit.

Mixed day8 goose/two-sheep could not fund entry. Delayedday9/day10 attempts
exposed an implementation restriction: entry required a natural harvest, but
those tiles had been replanted. compile_flexible now handles explicit crop
removal and mature harvests; new tests pending. Dates and crop removal must be
priced afresh, not assigned the oldday8score. Broader searches remain active.
