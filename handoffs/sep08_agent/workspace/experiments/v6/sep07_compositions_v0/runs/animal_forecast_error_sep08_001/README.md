# Animal investment forecast errors

Two previously exposed, independently completed courses were replayed again.
Both baseline action hashes/cash and candidate final cash match saved evidence.
The diagnostic uses actual future information only to locate errors; it does
not supply those values to a playable agent.

| Forecast stage | Early cow own gain | Early cow margin gain | Earlier geese own gain | Earlier geese margin gain |
| --- | ---: | ---: | ---: | ---: |
| Conditional shops, before labor | -457.03 | +4722.94 | +1032.59 | +1032.59 |
| Charge measured labor | -2088.03 | +3091.94 | +600.59 | +600.59 |
| Substitute actual future shops | -5889.00 | -1125.00 | +634.00 | +634.00 |
| Also use actual own daily trades/fixed costs | -5933.00 | -1169.00 | +586.00 | +586.00 |
| Also use actual rival daily trades/fixed costs | -4174.00 | -1072.00 | +506.00 | +587.00 |
| Exact full game | -4145.00 | -857.00 | +519.00 | +595.00 |

For early cows, the actual shop sequence makes the estimate much worse than the
conditional average. Replacing intended own trades with executed daily trades
changes it only44cash. Replacing the incomplete rival projection with actual
daily rival flows then changes it by1759cash, leaving29cash of own error and
215margin error. Thus intraday timing alone is not the principal remaining
error in this case. Unknown shops and rival flow assumptions are both material.
The effects depend on this substitution order and are not an additive causal
decomposition. The older model sees only the rival's current animal herd and
omits crop/fertilizer flows and future expansion.

For the goose course, measured labor already brings the estimate within81.59
of actual own gain. Actual daily flows reduce the residual to13cash/8margin.
These geese advance two already planned animals; they are not permanent herd
additions. Neither of these two cases establishes general estimator accuracy.

The existing public-current-crop helper was re-read: its biology was verified,
but prior fresh branch gains were small/uncertain and it cannot forecast later
planting or animal expansion. Do not insert it globally based on this result.
Next measure observation-only rival growth, fertilizer and trade-disposition
forecasts on multiple contexts, then rerun branch-ranking and runtime tests.

Reproduce with `conda run -n kaggriculture python
experiments/v6/sep07_compositions_v0/runs/animal_forecast_error_sep08_001/run.py`
in a clean output directory. PROTOCOL.json stores exact build/run commands and
input hashes. RESULTS.csv contains the unrounded estimates and actual results.
Raw diagnostic C++ is separate from all playable agent packages.
