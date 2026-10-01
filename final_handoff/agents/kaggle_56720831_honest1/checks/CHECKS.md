# pavel-bc-opus-v17d-dc12m19-honest-m68 (B: honest1): Kaggle bundle checks (Sep 30 22:12 UTC)

Not submitted. Kaggle kernel check pushed and passed (below). Not on the Local-LB.

Agent: Local-LB pavel-bc-opus-v17d-dc12m19-fc2-m68 (m19) with model.bin.dc11 without nearanimals=6 (splitfert=10 kept) and the
forecaster changed to BC's fc3nvens (model.bin.forecast_tf, .2, .3). Model files = experiments/v10/sep29_mm_copy/models/honest1
(byte-identical). Bridge = m19's (work/sep29_fund/src_m14 + m19 patch) with debug info stripped (same games; size cap). No CMA.
Package: packages/pavel-bc-opus-v17d-dc12m19-honest-m68 (29 shipped files match its BUILD.txt sha256 list).

Archive: submission.tar.gz, 92,224,179 bytes, sha256 b8ac48f8b3f717b5c90828912eaee16194f85870edcad58a77f75ecfe4487860
(main.py + libopus_lb_bridge.so + model/*; main.py equals the one in Kaggle 56706309 / 56714867).

- Official environment (kaggle_environments 1.32.7, main.py exec()'d as on Kaggle), unpacked archive (VERIFICATION.json): 4 / 4 DONE;
  self-play 105,466 twice (deterministic); vs starter 279,838 / 3,441 and 3,506 / 251,893 (= the package games); worst call 1.03 s
  (1 call over 1 s per agent in self-play); >= 59.97 s of the 60 s overage left. For scale: 56714867 (m14 bridge) was 0.98 s locally
  and 2.63 s on the Kaggle kernel, with >= 45.5 s overage left.
- Kernel source tarball (kaggle_check/dataset/source.tar.gz.bin) compiles with standalone CMake (kagbuild); the rebuilt bridge plays
  the same self-play game (105,466; kaggle_check/rebuild_test/rebuilt.json).
- Kaggle kernel check PASSED (Sep 30 22:38 UTC; pushed on the user's ask via Imitation, kernel only, no submission):
  pavelsavchenkov/kag-honest1-check v1 (dataset pavelsavchenkov/kag-honest1-sep30s), Kaggle image glibc 2.35, Xeon 2.20GHz x4, env 1.32.7.
  Archive sha asserted; 4 / 4 DONE; rewards equal local; rebuilt bridge (sha e6eefe4dc4e1) same rewards; worst call 3.52 s;
  overage left >= 44.8 s (kaggle_check/output/check_results.json).
- Kaggle-speed deadline emulation (runs/deadline, Sep 30 22:23 UTC; frozen full_games_dc11 b0 for m3's tree, b5 for m19's): 24 games vs
  d3crop (seed 501, both seats), FULL_BUDGET=2.7 (56714867: 0.98 s local -> 2.63 s on the Kaggle kernel), with / without the
  bridge soft deadline. The deadline changed 0 of 24 games; 0 dawns past the deadline; max overage use in one step 2.60 / 2.01 s (no deadline / deadline), overage left mean 49.1 s, min 30.9 s.
  Live m3 control in the same window: 0 changed, overage left mean 51.5 s, min 40.6 s.
