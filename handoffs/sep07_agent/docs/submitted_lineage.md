# Exact submitted agent and component lineage

The only official upload made in this session is **`shop_herd_s6_m3_g1`**, packaged as `sep7-shop-herd-adaptive-v1`, submission **56074695**, uploaded 2026-09-07 10:21:58 UTC and observed **COMPLETE**. The exact archive is [submission.tar.gz](../submitted/submission.tar.gz). The later local reference `investment_context_guarded_001_best` and the crop experiments were not submitted. See [development history](development_history.md) for those versions.

The official artifact is a standard-library Python deployment adapter generated from a frozen C++ policy. All local gameplay/search agents are C++. The adapter preserves the C++ decisions; it does not run the offline day solver or learn during a game.

## What the submitted policy does

It follows one 719-action Justin Lee replay course, with terminal inventory recovery, 22 locally rebuilt worker-day schedules, and two observed-shop cow/sheep choices. It chooses the two purchases at day 7 hour 0 using shops already revealed: count milk demand per shop-consumption cycle and wool demand per cycle, with each Yarn Store contributing two wool. Choose sheep if wool demand exceeds milk demand; choose cow if milk demand exceeds wool demand; ties keep the original cow. The addressed purchases occur at steps 169 and 176, with pickups at 175/180 and placements at 177/183. Day and step indices are the engine's zero-based indices.

Changing an animal also changes the traced purchase/pickup/placement and product handling. A day rebuilt by V30 is used only when its physical starting contract matches. Changed purchase/transfer days retain their addressed source course. A mismatch falls back to the original course; the guard does not prove future affordability. Terminal recovery redirects carried stock toward the shed and settles final sales, preserving market-order slots.

This submitted policy has observed-shop composition adaptation. It does **not** use the later general goose/cow/sheep/wait evaluator, the Atakan suffix portfolio, or an opponent-conditioned animal selector. Some included dependency headers are inherited infrastructure; inclusion alone does not make their policies active.

## Exact origins

| Component | Source and scope | Local change |
|---|---|---|
| Base farm, placement, dated services, market course | JustinLee, episode **106370340**, seat **0**, submission **56065461**, program **150**; global rank 10 in the 2026-09-07 08:02:02 snapshot | Normalized 719 recorded actions; no reconstruction of Justin's private branching code |
| Carried-stock recall idea | `yamakawanin/king-v4e-rc4`; Dusta latest-departure recall active in King RC4 | New typed terminal overlay over Justin's course |
| Final liquidation idea | Lynn Sakurai/Arlene, `lynnsakurai/farming-score-a-mathematical-approach`; source descends from the Thomas Tschinkel router and capacity layer | Preserve existing SELL slots, enlarge terminal quantities, append missing products; exact engine settles same-turn deposits |
| Worker-day reconstruction | Persistent repository **day_solver V30**, applied locally to successful Justin day contracts | Remove one hire, rebuild complete routes, validate exact endpoints and cash, then select compatible whole days |
| Milk/wool composition rule | The user's explicit intuition: more milk-demand shops should favor cows; more wool-demand shops should favor sheep | Mode 3 uses actual demand units, Yarn counts twice; two independently addressed purchase edits |
| Runtime and packaging | Local C++ API/engine, local exporter and Python adapter | Immutable data, independent episode state, exact packed-action parity and full official-environment games |

Authoritative nested provenance, including each day problem/schedule/header hash, is in the [frozen candidate IMPORT.json](../submitted/source_tree/experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/IMPORT.json). Its early `status: discovery only` field is historical; completed package validation is recorded separately in [DEPLOYMENT.json](../submitted/DEPLOYMENT.json). Preserve the frozen metadata rather than rewriting it to match later knowledge.

Selected day-library IDs are `0,1,2,4,5,7,9,10,11,12,13,14,15,16,17,18,20,21,22,23,24,25`. Their engine days, in the same order, are `16,18,20,11,15,19,22,23,24,25,26,27,13,14,8,9,6,7,0,3,5,2`. These are locally solved routes, not a claim that the original donor used V30.

| Input/artifact | SHA-256 |
|---|---|
| Justin episode 106370340 replay | `5284f2fdf9dfdebb5b694d2de797f2daf6b60ae13fcb898db6793983217536e2` |
| King RC4 source | `26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d` |
| Lynn extracted source | `1dc166ae2bf0c56a44fac4482f469b8812968c4cb32459cb9860f5077897a7d4` |
| Shared Thomas decoded route blob | `aabfbe9164937e4f46b59582479cebc5a1615a5652fa804adb172742f998db11` |
| Frozen `include/shop_herd.hpp` | `0490942269738a4a2523dcc06930765d7e01345f4f78e15c7ea5530b4a21fa2f` |
| Frozen `justin_day_library_001/days.hpp` | `a8bbd94bbb9729d86fe2dc8e942e09da0f49ab2bbaecac82d84293ad6f4ec62c` |
| Submitted `main.py` | `f5f58e14b0cafd0f150a505d81e63feb9426c8415771d23bde054b839f6b8d97` |
| Submitted `submission.tar.gz` | `53acf9d1d5e3206e28ea6f365af39786a9e74c4b3bfe6851b5bd21e8740901f4` |

[FROZEN.json](../submitted/FROZEN.json) identifies all 58 frozen source/dependency files. [ARTIFACTS.json](../submitted/ARTIFACTS.json) and [FINAL_SHA256SUMS.json](../submitted/FINAL_SHA256SUMS.json) identify deployment artifacts. The user authorized borrowing replay actions and public code. That authorization is recorded separately from source licensing: several retrieved sources did not supply a separate license, and their metadata preserves that fact. Public actions do not disclose a donor's private implementation.

## Validation of the exact submitted version

| Check | Result |
|---|---|
| Frozen C++ vs strongest teammate `shoprouter-rl-v2`, independent common shop stream | **3,980 wins / 4,096 games (97.17%)**, no ties; mean margin **+$9,426.50**. Seeds 1150000–1152047, both seats |
| Same frozen C++, official native RNG audit | **3,944 / 4,096 (96.29%)**, no ties; mean margin **+$9,307.88**. Separate scenario using the same seed interval |
| Extracted archive vs original teammate entry point, official environment | **62 / 64 (96.875%)**; mean margin **+$9,102.875**. Seeds 1153000–1153031, both seats; both final cash values equal frozen C++ in all 64 |
| Packed action parity | **48,892 actions** across the 64 opponent games plus four complete PASS/self games; all statuses DONE |
| Generic/release/debug/thread checks | 16 complete records identical; one and four threads; deterministic archive rebuild |
| Local official-runtime check | Maximum measured call **0.0414 s**; complete file-loader game DONE |
| Kaggle server validation episode 106421415 | Both DONE, rewards 66800/69098 reproduced, **1,438 actions** equal, empty agent logs; maximum server call **0.122497 s** |

These are local win rates against identified opponent versions, not a measured current leaderboard rating. [SUBMISSION.json](../submitted/SUBMISSION.json) records exactly one upload and its server receipt. The original reproducible commands and build hashes are in [DEPLOYMENT.json](../submitted/DEPLOYMENT.json); the handoff's portable scripts rebase the original machine paths. Server replay evidence is in [kaggle_validation/audit.json](../submitted/kaggle_validation/audit.json).
