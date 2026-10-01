# pavel-bc-opus-v17d-dc12m3-d3crop-m68, resubmitted (Kaggle 56720080, 2026-09-30 ~22:20 UTC, user-asked)

The exact archive of Kaggle 56690263 (sha256 f8c82bd813f7ab3e63adef8383a7990fd902f40f292045672d14e5105cf5ac17). This is our
most-tested submission: 152 live games, settled 2807.8, and the base of the Local-LB top entries. The original folder is
submissions/sep29d-bc-opus-v17d-dc12m3-d3crop-m68, and README_sep29_original.md is its README.

Why resubmitted: the overfit audit of Sep 30 (experiments/v10/sep29_mm_copy/LEDGER.md, AUDIT lines) found that the later subs
56706309 (CMA decode) and 56714867 (fc2) carry parts fitted to our own lineage. The user chose this base as a final pick.

Checks before the submit:
- Local, official env 1.32.7 (VERIFICATION_resub_sep30.json):
  - 4 / 4 DONE; rewards identical to Sep 29 (self-play 89,186 twice; vs starter 271,547 / 3,473 and 3,518 / 249,977);
  - worst call 0.76 s; 60 s overage left;
  - bridge needs GLIBC <= 2.27.
- Kaggle kernel pavelsavchenkov/kag-d3m68-m3-check, version 2 (kaggle_check/output/): same dataset, archive sha asserted; Kaggle
  image glibc 2.35, env 1.32.7.
  - 4 / 4 DONE with the shipped bridge; rewards equal local;
  - rebuilt-from-source bridge gives the same rewards;
  - worst call 2.59 s; overage left >= 44.1 s.
  - kaggle_check/output_sep29/ holds the Sep 29 run of the same kernel.

After the submit: Kaggle status COMPLETE (22:24 UTC). Validation episode 115990294 (self-play, seed 0; validation/) reproduces
locally exactly (audit_kaggle_validation.py): rewards 92,088 / 92,774 = Kaggle's; 1,438 actions, 0 mismatches.
