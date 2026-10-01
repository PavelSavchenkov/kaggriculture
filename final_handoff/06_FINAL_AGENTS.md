# 06 Final agents

| Folder in `agents/` | Agent id | Status |
|---|---|---|
| `kaggle_56720080_base_m3` | `pavel-bc-opus-v17d-dc12m3-d3crop-m68` | Kaggle 56720080 (final pick 1), re-submit of 56690263 |
| `kaggle_56720831_honest1` | `pavel-bc-opus-v17d-dc12m19-honest-m68` | Kaggle 56720831 (final pick 2) |
| `locallb_f1_fc2ens` | `pavel-bc-opus-v17d-dc12m19-fc2ens-m68` | Local-LB #1 (1651.2, 194-76); PR #248 |
| `locallb_m3_fc3nv` | `pavel-bc-opus-v17d-dc12m3-fc3nv-m68` | Local-LB #3 (1523.7, 145-125), the honest Local-LB entry |

## How they differ from the base

All four share the network `v17g6ft5`, the four members, the decode, the opening, the compiler switches and `main.py`.

| Part | base m3 | honest1 | f1 (Local-LB #1) | m3_fc3nv |
|---|---|---|---|---|
| Forecaster | `big2` | `fc3nvens` x3 (clean) | `fc2` x3 (has our games) | `fc3nvens` x3 |
| Compiler build | dc12 m3 | m19 build | m19 build | dc12 m3 |
| `splitfert=10` | off | **on** | on | off |
| `hirecheck=1 latehire=1` | off | **on** | on | off |
| `nearanimals=6` | off | off | **on** (lineage-only) | off |
| Days where play differs from base | — | days 3–9 (execution) + forecaster from ~day 10 | as honest1 + placement days 0–9 | forecaster only |

## Why these two for Kaggle

**Pick 1, the base.** It is our most-tested agent: 152 live games (more than any other submission), settled 2807.8, 12/22 wins
vs the top 10. It carries no fc2 data and no nearanimals. Its remaining lineage-selected parts are incumbents that played live
for days. The later submissions with lineage-fitted parts did worse live: CMA 2709, fc2 2727 in a harder pool.

**Pick 2, honest1.** It needed to be honest (no lineage-fitted parts) but more different from the base. The candidates were
"base + clean forecaster" (m3_fc3nv) and honest1. Evidence, paired per game:

| Bed | honest1 − m3_fc3nv | m3_fc3nv − base | honest1 − base |
|---|---|---|---|
| Live-game continuations, set A (25 games, 700 game-days) | +267 (SE 139) | +611 (237) | +878 (250) |
| Live-game continuations, **set B (81 fresh games, unused anywhere before)** | **+224 (SE 70)** | +32 (198) | +256 (210) |
| 5-day spans, set B | +187 (66) | — | — |
| Recorded real opponents (82 worlds) | +534 (292) | +409 (408) | +973 (386) |
| Hybrid opponent bed (66 worlds) | −404 (492) | +754 (480) | +350 (407) |
| Swap bed, real top-30 worlds (~126) | +613 (392) | +1,250 (411) | +1,904 (429) |
| Exact Local-LB (lineage; information only) | +1,314 (715) | — | — |

- honest1's edge over m3_fc3nv splits into splitfert +104 (SE 87) and the hire fix +105 (SE 78) on the clean forecaster.
  Neither part is negative.
- splitfert had flipped sign with fc2 (−70). fc3nvens predicts opponent morning milk like the base's own forecaster, the one
  place fc2 differs.
- Caveat: the forecaster step alone (m3_fc3nv vs base) did not replicate on the fresh set B (+32).

## Checks done on both Kaggle archives

Everything is in `agents/kaggle_*/checks/`.

| Check | base (56720080) | honest1 (56720831) |
|---|---|---|
| Archive sha256 | `f8c82bd8…` (= 56690263) | `b8ac48f8…` |
| Local official env 1.32.7, 4 games (self-play twice, both seats vs starter) | 4/4 DONE, rewards identical to the Sep 29 run, worst call 0.76 s | 4/4 DONE, deterministic (105,466 twice), worst call 1.85 s |
| Kaggle kernel (Kaggle image, env 1.32.7 installed offline; archive sha asserted; bridge rebuilt from source) | 4/4 DONE, rewards = local, rebuilt same, worst call 2.59 s, overage ≥ 44.1 s | 4/4 DONE, rewards = local, rebuilt same, worst call 3.52 s, overage ≥ 44.8 s |
| Kaggle-speed deadline emulation (24 games) | 0 games changed | 0 games changed (min overage 30.9 s) |
| Kaggle validation episode reproduced locally | 115990294: 92,088 / 92,774, 1,438 actions, 0 mismatches | 116002545: 82,127 / 81,798, 1,438 actions, 0 mismatches |

## Rebuild and re-verify

```bash
# runnable folder from sidecars + weights + bridge source (portable bridge: pass the kagbuild compiler)
conda run -n kaggriculture --no-capture-output python final_handoff/agents/assemble_agent.py kaggle_56720831_honest1 /tmp/h1 \
    --cxx ~/anaconda3/envs/kagbuild/bin/x86_64-conda-linux-gnu-c++
# exact archive: official-environment games
cd final_handoff/agents/kaggle_56720831_honest1
conda run -n kaggriculture --no-capture-output python verify_submission.py --archive pavel-bc-opus-v17d-dc12m19-honest-m68.tar.gz \
    --output /tmp/VERIFICATION.json
# reproduce Kaggle's validation episode from the gzipped replay (gunzip into checks/validation/ first)
conda run -n kaggriculture --no-capture-output python checks/audit_kaggle_validation.py checks/validation/episode-116002545-replay.json
```

`audit_kaggle_validation.py` expects the archive next to it. Run it from a folder that holds both, or edit `ARCHIVE`.

## Submission procedure we used (every time)

1. **Identity.** The package's model files are byte-identical to the evaluated model directory, and the package reproduces at
   least two judge / bed games exactly.
2. **Size.** The Local-LB caps agents at 104.86 MB. Extra forecaster seeds needed a debug-stripped bridge
   (`strip --strip-debug`); check that the stripped bridge reproduces the original's games.
3. **Portable bridge.** Build with the `kagbuild` conda compiler (glibc ≤ 2.27; Kaggle's image has 2.35).
4. **Local verify**: `verify_submission.py`, 4 official-environment games, deterministic, timing / overage.
5. **Kaggle kernel check.**
   - Dataset: archive + source + the env 1.32.7 wheel, uploaded as `*.bin`, because Kaggle auto-extracts archives.
   - Kernel: install 1.32.7 offline, assert the archive sha, play 4 games with the shipped bridge and a bridge rebuilt from
     source.
   - Use short kernel titles: long slugs gave HTTP 400.
6. **One submit**: `kaggle competitions submit kaggriculture -f <archive> -m "<id: what, evidence, checks, sha>"`. Team quota:
   5 per day. Only the 2 most recent submissions keep playing; each new one retires the older active one.
7. **After COMPLETE**: download the validation episode (`kaggle competitions episodes <id>`, then `replay <episode>`) and
   reproduce rewards and every action locally.
8. Store everything in `submissions/<date>-<id>/`, self-contained.

Local-LB PRs: branch `submit/<agent-id>` from the latest `origin/main` in a worktree of the Local-LB repo, run
`python -m lb validate --agent-dir agents/<id>` (with `PYTHONPATH=src`), check the BUILD.txt hashes, push. The LB worker
auto-merges and rates. `gh` is not installed, so give the `pull/new/<branch>` link.
