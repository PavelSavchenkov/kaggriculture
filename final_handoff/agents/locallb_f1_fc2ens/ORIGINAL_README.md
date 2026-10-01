# BC Opus v17d m19 + fc2 forecaster seed ensemble

Local-LB #1 `pavel-bc-opus-v17d-dc12m19-fc2-m68` with the opponent-sales forecaster averaged over three training seeds: #1's
fc2 (`model/model.bin.forecast_tf`, unchanged) plus `model.bin.forecast_tf.2` / `.3` (fc2 seeds 1 and 2; BC's f1_fc2ens). The
runtime already averages the three files; no code change. Network, members, decode, dc11 keys and main.py are #1's. No CMA.

The bridge is #1's with its debug info stripped, to stay under the 104.86 MB cap with the two extra files; #1's model files
with the stripped bridge reproduce #1's self-play game exactly.

## Checks

- Official environment (kaggle_environments 1.32.7, main.py exec()'d as on Kaggle): 4 / 4 DONE; self-play deterministic
  (138,208 twice); vs starter 146,194 / 3,668 and 3,539 / 236,703; slowest call 1.68 s; >= 57.8 s of the 60 s overage left.
- Local-LB validator (python -m lb validate, main's src/config): OK, 102,333,783 bytes.
- Evidence (Weaknesses' exact Local-LB judge, 90 games vs the top of the roster): score 76.4% vs 64.4% for #1's control copy,
  paired +891 (SE 393); vs #1 16-14, vs pavel-bc-opus-v17d-dc12m14-fc2-m68 27-3, vs pavel-mmpq-policy-v2 25-4.
