# 05 Models: networks and forecasters

All binaries are in `weights/` (each once; `weights/SHA256SUMS`). Training data is not included: it is large (forecaster
datasets alone are 8.9 GB) and is rebuilt from Kaggle replays with the scripts in `code/`. Original locations are named below.

## DayIntent networks (behaviour cloning)

**What they predict.** At each dawn, from the farm, market and history features, the network predicts a DayIntent:
- new crops / animals by type, land;
- per group of identical crops or animals, how many to water / fertilize / harvest / clear / feed / care / collect.

Counts are 101-way categoricals, decoded by exact MAP dynamic programming. Inputs are conditioned on player strength
(`.condition`). Imitating strong players' plans is the point.

| Network | File | Data / recipe | How it was chosen | Evidence |
|---|---|---|---|---|
| **`v17g6ft5`** (main, all final agents) | `weights/network/v17g6ft5/model.bin` (29 MB, width-512 family) | `v17w512s2` fine-tuned 4k steps at lr 5e-5 with goal schema 6, on arrays_v5 (x0.3) + fresh_sep26 + sep28b / c / d (Kaggle games of top players; at least 20 of 4,641 fresh rows are our own subs, i.e. 0.4%) | Seed 121 of 4 same-recipe seeds, picked on h2h vs econm6 + exact LB (lineage). Seed spread ±1k | Fresh mains +3–4k vs v12 (reactive). Real-opponent beds of the pick: level. Seed soups and the other 3 seeds did not beat it on real-opponent beds (Sep 30) |
| Members `w384b`, `sw100`, `mw5k`, `mw2k` | `weights/network/members/` (9–18 MB each) | v12-era networks (Sep 8–20 data), carried from package E (Kaggle 56621852) | Incumbent; keeping them rests on the exact LB + clone bed | Fresh members that restore top-team "waves" lost −2.1k (exact LB); dropping members on days 6–13 cost ~1.4k |
| `v12_cond` (history) | `pipeline_sep25_handoff/weights/v12_cond/` | 2.3M parameters, 1.45M days of top Kaggle players, strength-conditioned | The Sep 25 jump to 2740 | Was a top-decile seed of ~12 |
| `agent_sep23` (history) | `agent_sep23/model/` | 509k parameters, first DayIntent net | — | First BC + compiler agent |
| M&M copy networks (`copy_or_v5d`, `mm4q_pure`, ...) | Local-LB repo, `mm_copy_handoff_sep29/` | M&M's games, seed-blind and grid-blind variants | Copy fidelity gates G1 / G2 | Copy as main −2.1k; hybrid at weight 0.5 not better live |

Training code: `code/training_network/train.py` (latest copy, from `experiments/v10/sep29_bc_mm/scripts/`). Corpus builders:
`build_corpus_v5.py`, `build_fresh_corpus.py`, `build_conditions.py` (strength labels), `build_metadata.py`. Export to the
C++ format: `export_step.py`. Full history with every run: `experiments/v10/sep24_BC_opus/PROGRESS.md` (~4,100 lines) and
`pipeline_sep25_handoff/TRAINING.md`.

**Lessons for training networks:**
- Teacher-forced held-out loss does not rank game strength. Gate by games: single vs single, reactive beds.
- Strength labels must be per submission (`_rel`); fresh `_s150` tables mislabeled weak submissions.
- Any fine-tune of a deployed main costs 1–2k (perturbation). Change members or adapters instead.

## Opponent-sales forecasters

**What they predict.** For each product and each remaining hour, the opponent's expected sales. The market seller's
dynamic program uses this to choose what to sell when, and what the opponent's sales will do to prices. Architecture: a causal
transformer over the game's days (dawn head), plus an intra-day head that re-forecasts the remaining hours from what the
opponent has sold so far today, plus a next-morning head. The runtime averages `.forecast_tf`, `.forecast_tf.2`,
`.forecast_tf.3` (any number of seeds).

| Forecaster | File | Training data | Used in | Evidence |
|---|---|---|---|---|
| `big2_stk_xs` (Sep 27) | `weights/forecasters/big2_m3/forecast_tf.bin` | Kaggle top-LB + mid-rank games + our lineage as target (5x), stock / visible / xseen inputs; had stale data and a wrong hour-rotation augmentation | base m3 | The forecaster in the most-tested agent |
| **`fc2`** seeds 0 / 1 / 2 (Sep 30) | `weights/forecasters/fc2/` | All Kaggle ranks, Sep 27–30 x2, no synthetic lineage; `--shift 0`; yesterday's hourly sales of all 7 products. **Includes our own live games** (recent_ours = 47% of recent sequences, x2 like any recent group) | seed 0: Kaggle 56714867 and LB #2; 3 seeds: f1 (LB #1) | G3 +1.41k vs big2; live continuations fc2ens +574 vs package. Its exact-LB lead over clean forecasters is lineage fit |
| `fc3vens` | not included (Local-LB packages) | fc2 recipe minus all bed worlds (leak-free for the beds), keeps recent_ours | stack candidates | G3 +1.05k, swap +1.20k (leak-free), level vs fc2 on clean reads |
| **`fc3nvens`** seeds 0 / 1 / 2 (clean) | `weights/forecasters/fc3nvens/` | fc3 recipe **without our own agents' games** (`--exclude recent_ours --exclude-teams ours.csv`) and without bed worlds; clean checkpoint selection | honest1 (Kaggle 56720831), Local-LB `m3-fc3nv` | Equal to fc2ens on real opponents (+11, SE 226); on the base +611 (SE 237) live continuations, +1,313 hybrid; the most accurate on morning milk |
| Next-morning head | `weights/forecasters/next_morning_head/forecast_tf_next.bin` | `big2_nx` | all final agents | A new next-day head (fc_nx) added nothing (−0.09k) |
| `fclin` / `fclin_fresh` | not included | 6x weight on our own agents' games | none (dropped) | Led on the Local-LB; dropped by the user's lineage-overfit rule |

Training code: `code/training_forecaster/`.
- Trainer: `fc_tf_intra.py train <data dir> <name> ...`. Export to C++: `fc_tf_export.py`.
- Dataset builders: `build_train*.py`. Offline evaluation: `fc_eval.py`, `fc_avg.py` (seed averages), `fc_clean.py` (clean 800
  held-out games), `fc_groups.py` (by opponent rank).
- Exact recipes: `train_fc2seeds.sh` (fc2), `train_fc3nvseeds.sh` (fc3nvens):

```
fc_tf_intra.py train $D fc3nvs$s --shift 0 --weight-old 0.5 --stockin 1 --xseen 1 --val val_recent \
    --weight-teams $D/recent.csv:2 --xlag 1 --seed $s --exclude recent_ours --exclude-teams $D/ours.csv
```

**Lessons for forecasters:**
- Fix the training data and assumptions (stale windows, wrong augmentation, synthetic lineage), not the outputs with correction
  coefficients.
- Select by games, not NLL. Pinned real-game replays and live continuations rank forecasters; held-out NLL does not.
- **Never train on the bed's worlds or on your own agents' games** if you will judge on beds or leaderboards made of them. The
  gain will be memory of the opponent, not skill.
