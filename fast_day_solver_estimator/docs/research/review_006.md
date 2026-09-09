# Review006 — 2026-09-09 02:56 UTC

Decision: continue. Changed-plan examples improve ranking more reliably than simply making the source-day regressor larger. Prioritize direct marginal prediction and error-aware compute allocation, while the first unseen wave remains frozen and its reference work runs.

Evidence:

- Fresh development reference sweep is complete:683 physical contracts,657 with verified upper bounds after normalized-source merging,26 unresolved. No analytical deadline-impossibility flags among these valid source contracts. A longer1130-query pass across638 cases has started; it will tighten physical reference intervals without changing the frozen operational3-second results.
- Combined development has875 distinct physical contracts/784 upper bounds, including240 layout candidates. Keep all source days and derived candidates with their full team family. Exclude physical contracts shared across families from single-family model studies.
- On48 held-family layout pools, raw extra trees trained on source days alone choose a mean cold cost204.49 with seven failures. Adding other families' layout examples lowers that to119.35 with zero failures. The timing+residual tree model improves150.77 with one failure to121.21 with zero. The augmented timing formula is124.21; the earlier source-only formula remains119.90. Do not hide the methods that became worse.
- A monotone timing-basis tree did not beat the strongest alternatives in this first augmentation study. Keep the experiment as evidence; monotonicity alone does not solve ranking.
- The original additive score gives185.52 conditional top-one cost with four failures; original geometry arithmetic gives160.87 with two. Comparisons include a common analytical screen. These are development layout proxies, not whole-game gains.
- Actual-query-CPU replay gives the source-only timing formula10.064 seconds average to reach the best certified reference cost versus29.967 for original additive scoring and17.973 for original geometry. Against the original additive scorer, mean saved time has a family-bootstrap95% interval of10.15–30.20 seconds. Against original geometry it is approximately0.004–17.01 seconds. Charge modern prediction100 us/proposal and original scoring zero in this conservative comparison. Rare bad choices still affect fixed-budget cost differently from mean time-to-best; report both.
- The largest residual-model error involves Tarang222's day10: actual source workforce12, standardized short-sweep certificate16, nearby layout certificate10. Earlier hiring changes spawn positions. This is both a candidate-ranking issue and a reference/compiler-difficulty question;60 alternatives from12 diagnostic pools now receive270 deeper30-second queries.
- All five crops and all three animals occur in the fresh development inputs. Counts remain uneven and whole-team splitting does not prove independent strategy authorship. Additional independently constructed inputs and terminal-day handling remain coverage work.

Next20 minutes:

1. Audit new-wave overlap with frozen training and all exposed development inputs before reporting novelty.
2. Add direct cold/warm marginal error and pairwise ranking reports against original coefficient10 and geometry baselines.
3. Test a marginal model on related plans, then a gate from out-of-fold errors if its decision benefit justifies cost.
4. Start independent cold/terminal coverage work. Do not use pending validation outcomes to tune the frozen v1 candidate.

Next review03:16:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted final estimator or claim of faster full SOTA agent development yet.
