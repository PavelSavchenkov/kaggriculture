# BC Opus v17d m3 + clean fc3nvens forecaster

`pavel-bc-opus-v17d-dc12m3-d3crop-m68` (Kaggle 56690263) with only the opponent-sales forecaster replaced by BC's fc3nvens
(`model.bin.forecast_tf`, `.2`, `.3`; three seeds averaged; trained without our own agents' games and without the test-bed
worlds). m3's bridge already averages the extra forecaster files. Everything else is m3's (no CMA). Bridge: m3's with debug
info stripped for the size cap; with m3's own files it reproduces m3's four official games exactly.

## Checks

- Official environment (kaggle_environments 1.32.7): 4 / 4 DONE; self-play deterministic (87,469 twice); vs starter
  271,547 / 3,473 and 3,518 / 249,977 (= m3's); slowest call 1.20 s; >= 59.8 s of the 60 s overage left.
- Local-LB validator: OK, 101,969,439 bytes.
