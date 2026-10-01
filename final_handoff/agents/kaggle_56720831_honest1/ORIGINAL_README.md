# BC Opus v17d m19 "honest": no nearanimals, clean fc3nvens forecaster

`pavel-bc-opus-v17d-dc12m19-fc2-m68` with two changes aimed at Kaggle safety rather than our lineage:

- `model/model.bin.dc11`: `nearanimals=6` removed (`splitfert=10` kept);
- opponent-sales forecaster = BC's fc3nvens (`model.bin.forecast_tf`, `.2`, `.3`; three seeds averaged), trained without any of
  our own agents' games and without the test-bed worlds.

Network, members, decode (m3's, no CMA), other dc11 keys and main.py are unchanged. Bridge: m19's with debug info stripped
(size cap); it reproduces m19's self-play game exactly.

## Checks

- Official environment (kaggle_environments 1.32.7): 4 / 4 DONE; self-play deterministic (105,466 twice); vs starter
  279,838 / 3,441 and 3,506 / 251,893; slowest call 2.02 s; >= 58.2 s of the 60 s overage left.
- Local-LB validator: OK, 102,333,769 bytes.
