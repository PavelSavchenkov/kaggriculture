# Honest line: base and parts (Sep 30, 23:xx BST)

Honest base = `pavel-bc-opus-v17d-dc12m3-fc3nv-m68` (Local-LB branch submit/pavel-bc-opus-v17d-dc12m3-fc3nv-m68, 4d49aed; package
experiments/v10/sep30_pkg_improve/packages/pavel-bc-opus-v17d-dc12m3-fc3nv-m68; model = experiments/v10/sep29_mm_copy/models/m3_fc3nv).
It is the live Kaggle agent m3 (56690263; settled 2807.8, top 10 12 / 22 = 54.5%, ranks 11-30 63%) with one change: the
opponent-sales forecaster is BC's clean fc3nvens. No CMA, no RL, no nearanimals, no splitfert.

Evidence classes (sources: sep29_mm_copy/LEDGER.md AUDIT lines 22:14-22:24, Day compiler key provenance, BC network provenance):
- REAL = supported on real-opponent data (pinned real games, one-day live continuations, realistic hybrid bed);
- SAFETY = guard against a known failure (effect ~0 when it does not fire);
- LOCAL = supported on local / lineage beds only;
- FLAG = chosen on lineage beds / exact Local-LB only (overfit-prone by the user's rules);
- STRUCT = format or training default, not a tuned choice.

## Parts the honest base carries

| part | value | class | evidence |
|---|---|---|---|
| forecaster `model.bin.forecast_tf`, `.2`, `.3` | fc3nvens, 3 seeds averaged; no games of our agents, no test-bed worlds in training | REAL | BC symmetric one-day harness, 600 real-opponent game-days: +585 (SE 241) vs the package forecaster; exact LB -1,153 vs #1 (lineage fit, expected) |
| main network `model.bin` | v17g6ft5 | FLAG | seed pick among 4-6 same-recipe seeds (+-1k spread) on h2h vs econm6 + exact LB; real-opponent beds level / negative (RR 8 clones +0.24k, fresh clone -199, pinned live27 -300) |
| ensemble members | 4 E-era members (w384b, sw100, mw5k, mw2k) | FLAG (mild) | incumbent; keep decision rests on exact LB + clone bed |
| `.opening` | `6 7` (DSM style days 0-5) | FLAG | C++ mirrors vs our package + exact LB |
| `.condition` | `1 0.7500 1.0250` | STRUCT | training default |
| `.compiler` | sell_order 2 + 9 others | sell_order: REAL (general mechanism); others FLAG | C++ mirrors vs our packages; several inert |
| decode `reach 0.9 0.8 0.7` (days 6-14) | herd push | FLAG | lineage mirrors; neutral on real re-tune (and the CMA values of it are banned) |
| decode `max_land 3`, `v219 10` | day-10 fourth quadrant | REAL | real-opponent backed; removing v219 cost -3.2k on the league (own +1.4k, opponent +4.6k) |
| decode `earlycrop 4 6 8 2` | melons days 6-8 | REAL | 760 + wide duels |
| decode `earlycow 2 4 1` | early cow | REAL (weak) | weak; inert on the m3cma base (identical games without it) |
| decode `qpush 3 3 0.8 0.5` | day-3 crop push | mixed | mixed evidence; the qpush variant was inert on the m3cma base |
| dc11 `timing=0.5` | router deposit timing | REAL | panels +0.9-1.4k with the learned forecaster; never isolated live |
| dc11 `landtrim=4` | | REAL | neutral; removes NoLand collapses |
| dc11 `tieall=1` | sell now on ties | REAL | +1.6k on 79 top-30 pinned games; lineage-NEGATIVE |
| dc11 `nightfix=30` | Q4 night overflow repair | REAL | +0.75k on 40 real games (value from a local sweep) |
| dc11 `collectmin=5` | | REAL | pinned +240 (value local) |
| dc11 `dropany=6` | crew search | REAL | pinned +533, p < 1e-4 |
| dc11 `cashsell=1 wheatcash=1 reserve=0` | funding (m1) | REAL | pinned +1.5k; wide bed -1.24k (capped-sub artifact) |
| dc11 `startcomplete=1`, `survivalfloor=1` | | SAFETY | starved-opening cliff; unfunded-day collapse (-186k in 1 of 234) |
| dc11 `rival=1`, `scen=16`, `menu=3`, `saleslots=3`, `nighttrim=1` | | LOCAL | local beds only |
| dc11 `pricefloor=1 futurefloor=1` | | LOCAL (weakest) | never positive alone; hybrid: f1 without them +181 / -164 (noise) |
| bridge | m3's (bc_m3min: dc11 v95 + m1 / m2 / m3 patches), debug info stripped | STRUCT | stripped bridge + m3's files reproduce m3's 4 official games exactly |

## Parts deliberately left out

| part | why |
|---|---|
| CMA decode values (gio_cma / m3cma) | banned (user): lineage-tuned; m3cma live 2704 vs m3 2808 |
| fc2 / fc2ens forecaster (#1, f1) | fc2's LB lead = lineage fit (recent_ours 47% of recent sequences); fc3nvens - fc2ens +11 (SE 226) on real states |
| fclin | upweights our own games 6x (user rule: avoid lineage overfit) |
| nearanimals=6 | lineage-only gain: hybrid f1 without it +194 / +624; one-day live -106 (SE 74), 5-day -66 (SE 45) |
| splitfert=10 | not robust across bases: +320 (SE 101) on the m3 base, -70 (SE 136) on the live keys |
| hirecheck / latehire | SAFETY fixes, but they need the m14 tree (not in m3's bridge); +36 (SE 311) on G3 |
| c2 recency conditioning | exact-LB / hybrid positive but chosen on lineage-heavy judges; no real-opponent read |

## Next arms on this base

BC's main-seed arms: same package, only `model.bin` replaced (script: scripts/mk_honest_seed.sh). Judge them on real-opponent
beds (one-day live continuations, hybrid) before the exact LB.

Built (Sep 30 22:07 UTC, not pushed, not submitted; checks in LEDGER.md and kaggle_bundles/<id>/CHECKS.md):
- packages/pavel-bc-opus-v17d-m3fc3nv-soupg4-m68: main = BC's soupG4 (v17g6ft5 + b / c / d seeds averaged).
- packages/pavel-bc-opus-v17d-m3fc3nv-soupg45-m68: main = BC's soupG4E5 (those 4 + 5 e-seeds averaged).
- Kaggle archives + kernel-check folders: kaggle_bundles/<id> (scripts/mk_kernel_check.py).
