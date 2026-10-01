# pavel-bc-opus-v17d-dc12m19-honest-m68 = "honest1" (Kaggle 56720831, 2026-09-30 23:00 UTC, user-asked final pick 2)

This is Local-LB #2's m19 build with nearanimals=6 off. It uses BC's clean fc3nvens forecaster (3 seeds, trained without our own
agents' games or test-bed worlds) and keeps splitfert=10 and hirecheck / latehire. It has no fc2, CMA or RL parts.
The bundle was built by the Improve Agent (experiments/v10/sep30_pkg_improve/kaggle_bundles/pavel-bc-opus-v17d-dc12m19-honest-m68).
Archive sha256: b8ac48f8b3f717b5c90828912eaee16194f85870edcad58a77f75ecfe4487860.

Selection evidence (experiments/v10/sep29_mm_copy/LEDGER.md, Sep 30 23:xx lines):
- vs m3_fc3nv on continuations of our live Kaggle games (real opponents):
  - set A: +267 (SE 139);
  - fresh set B (81 games): +224 (SE 70);
  - 5-day spans: +187 (SE 66).
- recorded real opponents: +534 (SE 292).
- swap bed (real top-30 worlds): +613 (SE 392); vs the base m3 +1,904 (SE 429).

Checks:
- Local verify (VERIFICATION.json, VERIFICATION_imitation.json): 4 / 4 DONE, deterministic (self-play 105,466 twice).
- Kaggle kernel pavelsavchenkov/kag-honest1-check (kaggle_check/output):
  - archive sha asserted;
  - 4 / 4 DONE; rewards = local; rebuilt bridge same;
  - worst call 3.52 s; overage >= 44.8 s.
- Deadline emulation: 0 of 24 games changed.
- After the submit: Kaggle COMPLETE. Validation episode 116002545 (self-play, seed 0; validation/) reproduces locally exactly
  (audit_kaggle_validation.py): rewards 82,127 / 81,798; 1,438 actions; 0 mismatches.
