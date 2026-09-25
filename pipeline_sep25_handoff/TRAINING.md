# Training: data, network, reproduction

Commands assume a working experiment folder made by `setup_experiment.sh` (scripts expect
`experiments/<v>/<name>/` depth, `data/`, `models/`) and a build in `build/`. Run everything through
`conda run -n kaggriculture`. RAM rule: one training process at a time (each loads 24-32 GB of
arrays; three at once killed the machine's session).

## 1. Data sources

| Source | Script | Notes |
|---|---|---|
| Public replays of current top teams (Kaggle API) | `scripts/collect_replays.py` | selection frozen before download; rate-limited |
| Official daily episode datasets `kaggle/kaggriculture-episodes-<date>` (top ~600 episodes per day) | `scripts/import_daily.py` | every episode from Aug 16 replays exactly in our engine; Aug 14 and earlier use older rules (all mismatch) |
| Meta Kaggle `Episodes` + `EpisodeAgents`, Kaggle API listings | `scripts/build_metadata.py` | submission, ratings, dates; Meta Kaggle has no Submissions/Teams rows for a running competition |
| Replay DB `xishengfeng/kaggriculture-replay-db` (seeds + actions + daily snapshots) | `scripts/import_replay_db.py`, `tools/db_trace` | 41,423 verified episodes; made the network worse (section 5) |
| Other local replay collections | `scripts/import_local.py` | |

Replays become engine traces (`fast_game_engine/export_trace.py` format: config, engine version,
both seats' actions per turn, parity hashes); `tools/trace_check` verifies every trace against our
engine. Leaderboard ratings drift about 300 points over weeks, so a raw rating is not comparable
across dates (`scripts/build_conditions.py`).

## 2. Corpus v5 (what v12 was trained on)

- `experiment/data/corpus_v5.txt`: 48,465 perspectives (episode, seat, team id, submission id, split,
  trace path) from 29,367 episodes: every perspective whose submission reached a final rating >=
  2,700 after enough episodes, plus the earlier top-20 corpus (`scripts/build_corpus_v5.py`).
- Split by whole episode (both seats together): bucket = first 8 bytes (little endian) of
  SHA-256(`"sep24-BC-opus:<episode>"`) mod 10; 0 test, 1 validation, else train. 38,687 train, 4,812
  validation, 4,966 test perspectives; 1,453,768 perspective-days.
- `experiment/data/conditions_v5.csv`: per perspective, strength = submission final rating minus the
  ladder reference (90th percentile of rating_before over that date's official top episodes) on the
  date of its last episode, and the replay day index (days since Aug 15).
- `experiment/data/styles.json`: team id -> style index (1 M&M, 2 Majkel1337, 3 DECEM, 7 DSM, 8 Vadim
  Vasilenko, 9 Unknown Mother-Goose, ...; 0 = every other team).
- Traces: 4.8 GB unpacked. Download `training/data/traces_v5.tar.zst` from the team's private Kaggle
  dataset `pavelsavchenkov/kaggriculture-agent-zoo-sep25` (paths match `corpus_v5.txt`:
  `data/traces/<episode>.txt`), or rebuild them with the scripts above.

## 3. Labels, extraction and the coverage gate

`tools/extract` converts each perspective-day to its DayIntent label by replaying the recorded day
in the exact engine (`source/convert.cpp`), writes actor inputs (the same `source/features.hpp` the
live agent uses), targets and fixed-value masks. It never guesses: a day it cannot represent is a
failure and the tool exits 1 (`<prefix>.failures.csv`). Allowed ignores: more than one land quadrant
bought in a day, or an unplaced animal discarded. Corpus v5: 1,453,768 of 1,453,950 days represented,
0 failures.

```bash
mkdir -p data/arrays_v5
for s in $(seq 0 23); do build/extract data/corpus_v5.txt $s 24 data/arrays_v5/shard_$(printf %02d $s) & done; wait
conda run -n kaggriculture python scripts/grid_f16.py data/arrays_v5   # grids to float16; 22 GB total
```

The zoo dataset's extraction rebuilt shard 0 byte-identical to the arrays v12 was trained on.

## 4. Training v12_cond

```bash
conda run --no-capture-output -n kaggriculture python scripts/train.py --arrays data/arrays_v5 \
    --out models/v12_cond --steps 40000 --batch 512 --lr 1e-3 --width 256 --seed 0 --grid --marginal \
    --style --style-dropout 0.1 --condition data/conditions_v5.csv --condition-dropout 0.1 \
    --condition-strength 150 --condition-day 41 --valid-cap 30000
```

Exact arguments: `weights/v12_cond/history.json` (`args`), loss curve in `history`, log in
`train_log.txt`. 46 minutes on one RTX 5090 (data-loading bound); 1,160,464 training dawns; AdamW,
cosine schedule, gradient clip 1.0, strict FP32 (TF32 off). Final validation loss 17.229 at 40k steps
(global counts 2.42, land 0.013, crop total 1.06, crop share 3.70, animal total 0.18, animal share
0.23, one-shot options 5.41, retain 0.31, clear 0.11, fertilize 0.70, harvest 0.51, feed 1.25, care
0.62, collect 0.71). `train.py` writes `model.bin` (native FP32), `model.bin.features` (4) and
`model.bin.condition`; agents add `model.bin.opening` and `model.bin.compiler` by hand.

Other flags: `--style-v2` (unknown teams get index 31 and dropout means "no style"; fixes the
style-0 trap), `--augment` (dihedral grid transforms), `--min-strength`, `--since-day`, `--winners`
(filters; all lost), `--teams` (fine-tune on teams), `--init` (warm start), `--gpu-data` (arrays on the
GPU: 30k steps in 4 minutes for small data), `--search-opening "6 7"` (expert-iteration rows use the
opening style they were played with), `--overfit N` (memorization sanity check).

Checks before playing a new network:

1. Native parity: `tools/parity` + `scripts/parity.py` (100/100 identical decisions).
2. Teacher-forced decode check: `tools/decode_eval corpus out.csv` then `tools/decode_summary.py`
   (field totals vs labels, one-shot member mismatch; caught the DP bug at 86% mismatch).
3. Games (EVALUATION.md). Validation loss does not rank networks by strength (below).

## 5. What the data and training experiments showed

| Experiment | Result (games) |
|---|---|
| 13x data (v5 corpus) + strength/recency conditioning (v12) vs v11 | wins 87% -> 92% on 865 games; Local-LB 79% -> 88% |
| 10k vs 40k steps (all data) | -4.8k [-8.6k, -1.0k]; Local-LB 57% vs 88% |
| Only strength >= 0 (8.5% of dawns), 10k steps | -11.3k vs all data |
| Only winners / only recent (10k steps) | -4.3k / -2.6k |
| Style one-hot index 0 at inference | -6.6k (it means "average unknown team") |
| Strength input -100 / +300 instead of +150 | -2.5k / -2.8k on Local-LB |
| Dihedral augmentation (v12_aug) | +0.8k on the old panel, 18/40 in the mirror later |
| Width 384 (v13_w384b; valid 16.5 vs 17.2), all data (v14), width 512 (v15, stopped at 48k) | not better in games |
| Replay DB, all (v16_db, 80k steps) | -8.9k; median strength of that data -187 vs -116 |
| Replay DB, strength >= -116 only (v18) | 97/192 zoo vs v12 138/192 |
| v12 recipe, seeds 1-11 | 8, 19, 5, 20, 16, 9, 3, 7, 11, 23, 12 wins of 40 vs v12 |
| Fine-tune on strength >= 50 (800 steps, lr 1e-4) | 15-25 |
| Team fine-tunes (3k steps, lr 1e-4): DSM, DECEM, Goose, Majkel, Vadim, M&M | sparring partners; the Majkel/Vadim clones beat v12 head to head before the compiler fixes, not after |

Takeaways: data quantity and training length matter; filtering hurts; condition on strength instead.
Game strength varies a lot between seeds of the same recipe at equal validation loss, so train
several seeds and select by games (v12 itself is a lucky draw).

## 6. Fine-tuning recipes

- Team clone (`scripts/zoo.sh`): `--init models/v12_cond/model.pt --teams <team id> --lr 1e-4
  --steps 3000`, then write the team's style index to `model.bin.style`. About 5 minutes each.
- Expert iteration (`scripts/expert_iter2.sh` -> `scripts/build_search_corpus.py` ->
  `scripts/ei2_finetune.sh`): `search_games` plays our agent against exact copies of our own lineage,
  searching 7 decision pushes per dawn with full-game rollouts up to `SEARCH_LAST_DAY`; our seat of
  each searched game becomes BC labels (team id 3e9+, style 30), mixed ~10x with arrays_v5, v12
  fine-tuned at lr 1e-4. A run was generating data at the snapshot (13 searched dawns x 7 rollouts =
  30-60 minutes per game per thread; `SEARCH_SEAT` plays one seat per seed because mirrored seats
  repeat the same game). The first attempt (5 pairs, 239 changed days of 896) was too small.

## 7. Value model (tried, not usable yet)

`scripts/train_value.py` (same encoders; heads: margin/20,000 Huber and win logistic),
`scripts/build_value_corpus.py`, `scripts/value_decision_test.py`, `scripts/value_data.sh`. Trained on
replay dawns: held-out R^2 0.36-0.58 on days 3-9, 0.80 by day 18, win accuracy 0.67 on day 5. The
decision test (exact-copy rollouts of the 7 decision pushes on 32 dawns): argmax V regret $3.4k vs the
baseline's $1.4k and random $2.5k. A version fine-tuned on 600 on-policy perturbed games (valid R^2
0.47) did not change that. Next-dawn value does not rank decisions whose effects appear days later.
Design notes from the companion study: `docs/weakness_analysis/VALUE_MODEL.md`.
