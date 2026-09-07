# September 7 adaptive herd submission

Frozen policy: `shop_herd_s6_m3_g1` from this session. The authoritative C++ package and all build dependencies are in `source_tree/`; `FROZEN.json` records their exact hashes. No runtime source outside this directory is required.

The production course comes from Justin Lee's public replay, program 150. Local changes recover final carried stock, liquidate final products, reuse 22 physically guarded day schedules with fewer hires, and choose cows or sheep for two day-7 purchases from already observed shop demand. Yarn demand counts twice; ties keep cows. Full component and idea attribution is in the frozen package's `IMPORT.json`. The package's original discovery status is preserved; subsequent validation is recorded in `DEPLOYMENT.json` and `source_session_validation.json`.

`export_policy.cpp` exports the one reachable course and the 22 exact day contracts/routes. `build_submission.py` encodes those tables and the bounded observed-state rules in `main.py`. The archive contains only `main.py`, at its root, and needs only the Python standard library. Empty market slots remain `PASS` so simultaneous order positions stay equal to C++. The adapter preserves the C++ policy's decisions, including fallback to its original course when a physical day contract fails.

Validation before upload:

- Frozen-source C++ versus teammate `shoprouter-rl-v2`: **3,980 wins in 4,096 games (97.17%)**, no ties, mean cash margin **+$9,426.5**. Fresh seeds 1,150,000–1,152,047, both seats, independent shop stream. `frozen_cpp_4096.json` contains every outcome.
- Extracted packed submission versus the original teammate entry point: **62 wins in 64 official-environment games (96.875%)**, mean margin **+$9,102.875**. Seeds 1,153,000–1,153,031, both seats, official native shops and weeds. Both terminal cash values exactly match the frozen C++ run in all 64 games.
- **48,892 actions match frozen C++** across those 64 matches plus four PASS/self-play matches. Every status is `DONE`. Separate modules hold each player's episode state.
- The validated original generic binary and frozen release/debug binaries match 16 complete outcome records, including action hashes, using four versus one worker threads.
- The official file-path loader completes self-play with `DONE` for both players. Maximum measured policy call was 0.0414 seconds against the one-second action limit; all calls in any one game totaled at most 0.0504 seconds.
- Rebuilding produces identical artifact hashes. `DEPLOYMENT.json` records commands, hashes, compiler version, and check results.

Build and run all Python/C++/Kaggle commands through `conda run -n kaggriculture`. The competition upload and terminal validation status are recorded separately in `SUBMISSION.json` after upload. Exactly one upload is authorized for this task.

After upload, while Kaggle validation was pending, the same frozen C++ policy also won **3,944/4,096 games (96.29%)** with the official native shop/weed RNG, using the same seed range. This confirms that the large-game win rate remains strong with official shop generation. Results are in `frozen_cpp_native4096.json`; the submitted artifact did not change.

Kaggle submission **56074695** is **COMPLETE**. Exactly one archive was uploaded. Validation episode **106421415** completed with both players `DONE`; local reproduction matched both final rewards ($66,800 and $69,098) and all **1,438 server actions**. Both agent logs are empty of stdout/stderr. The largest server call was **0.122497 seconds**, and each player's 719 calls totaled less than 0.163 seconds. The replay, logs, and audit are in `kaggle_validation/`.
