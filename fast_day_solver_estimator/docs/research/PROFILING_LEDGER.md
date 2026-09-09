# Profiling and verification findings

- A certified source schedule gives an upper bound on necessary workforce. It may waste labor.
- UNKNOWN is a bounded-search outcome, never proof of infeasibility.
- Existing `recorded_support` changes both labor and land. Its calibration gain is not an isolated labor ablation.
- The latest flat operation model omits geometry, deadlines and nonlinear hire thresholds.
- Public DayProblem v3 fixes dawn state, buys/hires and withdrawals. Source worker count and hire count must be stripped from estimator features.
- Terminal day has only 23 actual worker phases. Derived restrictions added outside public JSON may not survive reloading; verify directly.
- Physical solver success excludes cash/capacity/rival prices. Full-game integration must check them separately.
- Other machine jobs are active. Record CPU and wall time; leave unrelated processes untouched.

Next: inventory reference inputs, available accepted schedules, exact duplicates and family ancestry before selecting benchmark membership.

01:56 findings:

- Checking2,253 source witness pairs found no task/travel/shell lower bound above a valid source workforce. This empirical check supports but does not replace the bound argument.
- Earlier hiring does not preserve old worker spawn squares: `Sim::do_hire` chooses by current shed occupancy.536 historical and309 fresh normalized witnesses pass strict replay; all others remain rejected.
- One StandardScaler+ridge historical transfer produced MAE13,805 workers. Do not hide this with an output cap; inspect near-constant feature scaling and extrapolation.
- No repeated or warmed inference timing yet. Initial extraction-only numbers are not an end-to-end latency claim.

02:17 findings:

- Workers carry unlimited items. The old division by12 is a transport heuristic, not a carrying rule. Remaining cargo transfers automatically after hour23; only timed withdrawals/internal reuse force earlier trips.
- Precomputing purchase availability preserves all233 feature values on685 cases. Repeated-input feature+bound p50/p95 is26.087/44.889 us; this does not replace mixed-input timing.
- All2,253 historical/fresh witness pairs were checked against the stronger deadline conditions. No valid source contradicted the bound or reachability screen.
- C++ candidate export contains14,408 nodes/128 trees plus12-term and233-term linear models. Measure actual inference and prediction parity before calling it deployable.

03:56 findings:

- Frozen C++ source-wave mean/p95 scoring time58.32/81.0 us, layout-wave82.07/107.65 us, independent sparse/dense corpus44.81/149.27 us. The complete call includes features, the necessary bound and all three exported models; parsing/output are excluded. Means and tails differ substantially by work size.
- Python/C++ parity remains below8e-9 workers on685 original development inputs,591 validation inputs,240 validation layouts and289 independent cold inputs. Frozen binaries and source snapshots preserve the first version despite later header edits.
- First-wave source macro workforce error improves59.99% over tuned task-only, but the primary has no measured time-to-quality gain over original geometry on novel layout pools. Ranking/operational results must accompany point-error reports.
- Terminal controls reject required work in the nonexistent phase23 and accept removing an unnecessary final transfer. Remaining cargo is a terminal total, not real shed inventory or sale proceeds.

04:55 findings:

- Additional route-order optimization costs 491–648 us on the current repeated-input measurements and gives no useful consistent decision gain. Keep its negative evidence; exclude it from the default candidate for now.
- Conservative carried-input release bounds cost mean 1.065 us / p95 1.543 us over 6,205 loaded input variants. This includes hire-menu construction and excludes JSON loading, invariance checks and witness replay. It adds 39 synthetic and ten development layout impossibility proofs, with no contradictions on 4,445 valid witnesses.
- Work-removal marginal gains persist in the baseline-cost slices, but only five certified pairs from two parents have baseline cost at least 500. In that sparse tail, original flat10 MAE is 195.8, timing_boost 123.59 and timing_extra 167.93. These are exposed development comparisons, not strong evidence about arbitrary expensive expansions.
- A compiler-call success classifier is now evaluated with full parent-family exclusion. The requested workforce is a legitimate action argument; source workforce, reference answers and unqueried outcomes are excluded. Classification metrics are not enough: measure policy decisions against simple query-order/pruning controls.

05:56 findings:

- Whole-team splitting still allows exact overlap: the second wave contains 30 previously exposed source contracts. Remove whole overlapping-parent proposal pools for the primary novel analysis.
- A 30-second adjudication pass improves 31 synthetic upper bounds and adds three certificates, while the first model retains a large negative bias. Report label sensitivity alongside signed errors.
- The latest original general compiler allows six extra hires and preserves inherited hire times. Its fixed-repair stage uses up to one second and four internal workers before the one-worker cold fallback. Summed reported solver wall times omit other compiler work; measure complete CPU and wall time explicitly.
- Source season reconstruction matches archived cash and both action hashes across 719 actual transitions. Thirty day-contract witnesses replay strictly, but terminal predictions remain unsupported by the current ordinary-day model.

06:36 findings:

- Independent complete-season replay confirms the new wheat certificates and economic traces. Same physical obligations can have very different first-found compiled hire bills; the archived 10,425 marginal bill falls to 3,660 in the matched-prefix rerun. Keep this as reference-quality evidence.
- New expansion generation preserves actual dawn states and original work, buying only the additional inputs. Many dense farms have no free owned tiles; a separate one-/two-asset selection is needed rather than silently reducing a requested larger expansion.
- A local endpoint simulation can define a new plant/animal's state without certifying that workers can reach it or obtain inputs globally. Require stock conservation and preserve failed global reference calls.
- Release bounds must allow workers to preposition before land/seed availability and allow multiple workers to perform consecutive tasks on the same tile in one phase. Chain length alone is not a serial hour bound.

06:56 findings:

- Seed/land release computation costs 1.863 us mean and 2.824 us p95 across 6,205 variants. It passes 4,445 valid witnesses but adds no older-panel pruning. This is a negative usefulness result, despite low latency.
- An explicit hiring menu must be part of reference identity. Identical obligations under early and late hiring cannot share a workforce label. The new feature path ignores source crew/hires and passes permutation checks on all 160 calendar inputs.
- The calendar capacity helper matches 119,104 direct action-count and invalid-slot controls. The existing cost forest still has no terminal support; capacity transfer is a separately named heuristic, not an implicit claim of full horizon generalization.

07:16 findings:

- Mandatory hires and optional early hires require separate selection semantics. Selection order is not actual worker numbering; capacities use the selected birth-time multiset. Mandatory last-phase hires cost money but have zero action capacity.
- Exact-query screens pass 4,543 valid witness checks with no rejection. Average cost is 74.477 us, p95 118.031 us, including physical feature extraction and calendar validation; checks can share physical cases.
- Complete warm-compiler benchmarking now includes both successful and failed proposed courses. Two repeats reverse each course's method order; independently replay successful results and report bill/cash changes with CPU differences. Extra failures cannot become speed gains.
- A deliberately duplicated proposal alias changes raw membership count while leaving primary marginal metrics and paired confidence intervals exactly unchanged.

07:36 findings:

- A cold-call forecast must ignore unselected hire slots and the mandatory/optional designation of the same selected hires. Exact C++/Python parity holds on 6,400 320-feature queries, including these invariants.
- The current cost forest's labeled training range is two through seventeen workers; 99 percent are at fourteen or below. Keep that coverage limit visible when discussing expensive unseen cases.
- With almost no skipped calls, partial warm runtime differences mainly expose finite-budget repair variance. Preserve repeated paired controls and full quality checks.
- Removing virtual phase 23 after a 24-phase solve can censor terminal certificates unnecessarily. Add explicit required-work deadlines before solving in a separate reference version; never overwrite first-pass outcomes.

07:56 findings:

- Explicit phase-22 work constraints preserve all 45 verified terminal witnesses and pass independent boundary/solver controls. Their new timed rounds are separately identified from the old wrapper.
- Source-certificate transfer is calendar-specific: only three of fourteen attempted transfers pass strict replay. Those three improve twelve to eight/nine workers and add a thirteen-worker upper bound. Never transfer labels from obligation identity alone.
- A best-known certificate and a fresh timed solver outcome answer different questions. Keep physical upper bounds and short-sweep fields separate so stronger evidence does not rewrite measured compiler behavior.

08:16 findings:

- Small additions: 168 known pairs, 21 censored. The three cost increases of at least 500 remain grossly underestimated; two have the wrong sign under the direct forest. They come from only two parents.
- Larger additions: 134 known pairs, 63 censored. Timing plus boost is stronger than direct-cost trees, including on 17 increases of at least 500 from eight parents.
- Complete warm-screen benchmark: two skips in 568 calls, both in failed cow runs. Finite-budget repair variance dominates the 0.0673 seconds of total screen overhead; the gate fails and no speedup is claimed.
- Every one of the 31,110 reused ordinary reference calls has the expected input-derived hire prefix. Exact context feature parity holds for all 40 requested workforces on 1,739 physical contracts.

08:36 findings:

- Removing two overlapping layout pools changes the optional CPU hybrid from an apparent substantial full-panel gain to only 1.30%, with an interval crossing zero. Novelty filtering changes the decision.
- The primary direct-cost nearby policy passes all five fixed gates on 46 novel pools. Its worst material regressions are 3.464, 1.871 and 1.129 seconds, with unchanged 30-second bills.
- Time to reference-best cost cannot be an observed stopping condition. At first certificate, direct bills are much better than geometry, but the saved-time interval crosses zero.
- Before any certificate, the original hybrid gain is constant one; high-workforce success probability can dominate price. Cost discounts reduce that error but remain development candidates.
- Explicit terminal work deadlines yield thirteen calendar certificates versus ten initially. Keep the union of valid physical evidence while preserving each operational backend round.

08:56 findings:

- Terminal physical upper evidence must include earlier wrappers and source certificates: one new-round 25-worker result has a valid 13-worker source alternative. Four physical bounds improve without changing operational labels.
- Calendar descriptors without calendar training examples create severe overconfidence. Full-family exclusion with varied calendars reduces boosted Brier to 0.0710 and signed probability error to +0.0138.
- At 33-40 workers the old query model predicts 96.65% success; only 21 of 1,592 calls succeed. No 37-40 call succeeds in the novel-layout panel. This is budgeted backend behavior, not infeasibility.
- The C++ context path matches Python on 225,000 queries with errors below 1.3e-14; extra call-model compute is only a few microseconds. Better calibration still needs to translate into better decisions.

10:37: Record full per-query physical inputs as well as final course outcomes. Identical warm inputs produced different solver/endpoint outcomes in eight common calls; a final bill difference alone does not prove the estimator skipped a successful cheaper call. Retain failed predeclared gates even when the diagnosis suggests backend variance. Third-wave exposure inventory includes 5,309 physical keys and 160 calendar contracts.

10:56: Completed physical adjudication reports must explicitly select reference_workers. A missing target flag reproduced the unchanged short-budget target; preserve that report with a correction and write the intended physical report separately. The panel projection reader now retains multiple variant aliases and exactly preserves prior counts and signed metrics across 228 contracts and 244 raw memberships.

Final findings: audit the whole distribution and actual model support. The 925-row direct-cost fit has eight labels at fifteen-plus workers and no label above seventeen; every tree leaf is at most1,537.75 cash. 125/128 unchanged leaves explain the worst small-addition near-zero response. A range-only guard misses severe in-range combinations. Exact high-count seed-release constructions supply240 lower-bound-matched witnesses, but broad promotion is still pending. Package relocation must include relative-header dependencies and complete native runtimes; full-course action parity is required after namespace/include changes.
