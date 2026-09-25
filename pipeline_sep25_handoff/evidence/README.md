# Evidence: every game result of the experiment

Built from `experiments/v10/sep24_BC_opus/reports/` at the snapshot. One row per game, from our
agent's side (for mirrors: the candidate's side). Columns: `set` (the original report folder, which
names the run), `file` (the run's CSV name or the opponent), then the tool's columns.

| File | Rows from | Key columns |
|---|---|---|
| `summary.csv` | one row per run and opponent: category, set, file, reference (mirror opponent), games, wins, losses, draws, mean_margin | |
| `cpp_mirror.csv` | `candmirror/` (candidate vs reference, file `<cand>_<seed set>`), `knobmirror/`, `netmirror/`, `sellorder/`, `abl_Y/` (fin_Y ablations), `mirror1/` (one-seat screens vs fin_I), `h2h1/` (Y+rs vs Z+rs), `q4probe/` (4th-quadrant probes) | seed, seat, own, rival, margin |
| `cpp_panels.csv` | C++ opponents per candidate (`panel/<name>`, `screen/<name>`), zoo league (`zoo/<compiler>`), early gates (`final`, `final2`) | + fallback_days, discarded, compile_ms_max, land, animals |
| `frozen_replays.csv` | frozen top-10 replay gate per candidate (`panel/<name>/replay.csv`) | trace, replayed_seat, own, rival, original_* |
| `search_space.csv` | per-dawn search headroom vs exact opponent copies (base / network pushes / compiler variants) | searched_days, changed_days, choices |
| `python_harness.csv` | every official-engine game: Local-LB panels (`lb10/*`, `screen/*/lb`, `panel/*/lb`), exact Local-LB replays (`lbseeds*`, `roster10`, `roster_all`), head to heads (`h2h/*`), PPO panels, extra agents | opponent, seed, seat, own, rival, margin, max_action_seconds, fallback_days, compile_ms_max |

Name mapping (model folders to agents): LINEAGE.md section 6. Examples:

- Exact Local-LB replays: `lbseeds/base` = vadim (333/400), `lbseeds/herd`, `lbseeds2/robust`,
  `lbseeds3/so2` = slots, `lbseeds3/so2rw` and `lbseeds4/so2rw` = wages, `roster10/fin_I` and
  `roster_all/fin_I` = forecast.
- Invalid sets (model-path leak, before Sep 25 15:25): `h2h/c2a_old`, `h2h/c2b`, `h2h/c2c`,
  `h2h/cand1`. Valid head to heads: `h2h/c2a`, `h2h/sp`, `h2h/c2csp`, `h2h/so2_ppo`, `h2h/so2rw_ppo`.
- Runs of the 4th-quadrant probes (`q4probe`) were still in progress at the snapshot.

Paired comparison with a seed-clustered bootstrap (as in RESULTS.md):

```python
import csv, collections, random, statistics as st
rows = list(csv.DictReader(open("cpp_panels.csv")))
games = collections.defaultdict(dict)
for r in rows:
    games[r["set"]][(r["file"], r["seed"], r["seat"])] = float(r["margin"])
a, b = games["panel/cpp_fin_I"], games["panel/cpp_fin_T"]
keys = [k for k in b if k in a]
clusters = collections.defaultdict(list)
for k in keys:
    clusters[k[:2]].append(b[k] - a[k])
values = list(clusters.values())
boot = sorted(st.mean([x for c in random.choices(values, k=len(values)) for x in c]) for _ in range(2000))
print(len(keys), st.mean(b[k] - a[k] for k in keys), boot[50], boot[1949])
```
