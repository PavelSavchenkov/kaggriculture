# Review004 — 2026-09-09 02:17 UTC

Decision: continue. New features based on actual carrying, delivery and routing rules produce enough measured improvement to justify the first frozen C++ candidate and a new-family validation wave. The next priority is error direction and decisions on unseen plans, not a larger undirected model sweep.

Evidence:

- First historical sweep complete:400 contracts/3,523 queries.392 have an upper bound after merging strictly normalized source witnesses;8 remain without a certificate. One has an independently unreachable delivery. Source witnesses improve9 short-sweep counts, by up to2 workers, and rescue2 cases without short-sweep certificates.
- A longer unchanged-V30 pass now searches747 selected workforce thresholds across394 historical cases at30 seconds each. Previous certificates remain available. This is reference tightening, not an estimator contribution.
- Fresh development model study uses282 single-family contracts with completed reference sweeps and verified upper bounds. Across8 held-family folds, macro workforce MAE: tuned task-only0.9003, old fitted geometry0.9327, new timing/routing formula0.5969, stable ridge0.4502, raw extra trees0.4326, timing formula plus residual trees0.3765. Labels are upper bounds, not proven optima.
- Historical transfer is harder: the new timing formula is worse than old geometry when trained only on root cases and tested on the archive. Keep this as a distribution-shift failure, not an omitted result. The stable ridge removes the catastrophic scaling explosion but is still weak on that transfer.
- The new necessary deadline test contradicts no verified source witness and detects47/240 changed layouts with unreachable timed deliveries. It is a separate analytical screen, not learned feasibility.
- Repeated-input profiling over685 fresh inputs: feature plus bound median26.087 us/p9544.889 us. Mixed-input, end-to-end model timing is next. Exact features match before/after removing repeated purchase scans.
- Exported a12-term formula,233-term ridge and128-tree residual model with14,408 nodes to fixed C++ arrays. Build succeeds; numeric parity and mixed-input timing are not yet checked.

User steering: inference must stay cheap and efficient C++; keep new diverse data unseen until evaluation; track over/underestimates and hard cases; allow a measured gate to a somewhat heavier C++ model. These requirements remain active.

Next20 minutes:

1. Verify exported predictions against Python and measure mixed-input feature+model latency.
2. Freeze the first unseen-wave protocol and candidate before downloading new families. Keep later reserves unopened.
3. Add signed error slices and explicit hard-case records; test a gate using only input/model uncertainty, never reference answers at inference.
4. Evaluate matched layout choice with common analytical screening and tuned geometric baselines. Report marginal and warm-baseline behavior separately.

Next review02:36:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted model or submission.
