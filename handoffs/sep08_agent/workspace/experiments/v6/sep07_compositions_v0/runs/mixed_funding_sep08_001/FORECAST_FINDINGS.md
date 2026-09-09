# Mixed-course forecast attribution

This is an offline diagnostic for day9, seed1008, changing tile5 to goose and
tiles7/8 to sheep. It is one fixed-world course, not a promoted playable policy.

The complete compiler and independent audit cover all30 day endpoints and
719 live transitions. Own cash falls4923; rival cash falls4956; margin gains33.
Labor rises1563. Production changes, in engine product order, are
[-16,-7,0,-14,0,36,0,44,57].

The first forecast missed the output of crops already growing when removed.
V2 includes that displaced output, treats their planting cost as sunk, preserves
an explicitly required entry harvest, and disables new collection on day29.
All9 production deltas then match exactly. Three saved fertilizer inputs are
separate from the57 extra fertilizer produced. This fixes diagnostic accounting;
it has not yet been integrated into every runtime estimator.

| V2 information supplied | Own cash delta | Rival cash delta | Margin delta |
| --- | ---: | ---: | ---: |
| Conditional shops, before labor | 6214.53 | -762.50 | 6977.03 |
| Conditional shops, measured labor | 4651.53 | -762.50 | 5414.03 |
| Actual shops, measured labor | 1340 | -1320 | 2660 |
| Actual shops and own flows, fixed costs | 1362 | -1320 | 2682 |
| Actual shops and both players' flows | -4403 | -4720 | 317 |
| Exact full game | -4923 | -4956 | 33 |

After the biology fix and actual shops, own-flow approximation differs by22.
The largest remaining miss is the opponent's future flow response. Actual-shop
and actual-flow rows use offline oracle data only to locate error; they are not
deployable estimates. Next: model public opponent growth and price response,
and validate on untouched worlds before using the estimate for selection.

Keep FORECAST.csv/FORECAST_PROTOCOL.json as the historical V1 result and
FORECAST_V2.csv/FORECAST_V2_PROTOCOL.json as V2. Adding the V2 executable appended
a CMake target after V1, so the V1 CMake hash describes the earlier build file.
Other frozen diagnostic source inputs remain independently identifiable.
