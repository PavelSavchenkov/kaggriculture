# Positive and negative learnings

## What worked

- Train and evaluate on changed proposals, keeping entire parent families together. Ordinary source-day MAE alone did not select the best layout policy.
- Direct hire-cost training handles nonlinear Fibonacci prices better than blindly converting a mean worker prediction. The accepted forest is fast and improves ordinary cold ordering and removal estimates.
- Include temporal structure: goods releases, withdrawals, required pickups, exact selected hire dates, mandatory hires and the active horizon. Calendar features only transfer well when varied calendars appear in training.
- Keep minimum physical workforce separate from bounded compiler success and CPU. A larger workforce can be harder for a time-limited backend, so solver success need not be monotone in worker count.
- Measure complete compiler runtime. Learned deferral can save meaningful CPU while retaining fallback, even when a necessary-only screen rarely fires.
- Preserve every valid schedule when strengthening physical upper bounds. Longer references reduced many old errors and also exposed previously unlabeled expensive failures.
- Use exact input hashes, family exclusions and C++ choice parity. Alias duplicates and overlap filtering materially change apparent gains.
- Use a cheap operational difficulty probe at the proposed point before computing the full curve. Average compute matters more than enforcing identical time on every input.

## What failed or remains unproven

- The first model did not pass the stronger cold comparison. Keep its failed gate; later progress does not erase it.
- A more expensive routing feature, at about 0.5–0.65 ms, did not add enough decision value. It is not a default feature.
- Direct pair regressors and monotone timing models did not beat the strongest cheap alternatives on the tested grouped decisions.
- Optional query hybrids did not add reliable gain after novelty filtering. One appeared useful on the full panel but improved only 1.30% on novel pools, with an interval crossing zero.
- Replacing ordinary ranking by a curve-derived worker score made layout search slower despite reasonable point-error metrics.
- The necessary-only warm screen skipped two of 568 calls, was 4.35% slower and raised mean bills. Reject it as a standalone speed optimization.
- The first learned warm test lost a certificate despite faster CPU. Later new-seed success is scoped evidence, not a reason to hide that failure.
- A richer conditional retry forest improved Brier error but not decisions; it could miss rare first certificates. The simpler existing-model refinement cutoff remains the prospective candidate.
- Known-baseline worker anchoring raises owned-addition MAE. A baseline upper bill can itself be loose; it is not a clean estimate of intrinsic difficulty.
- Training-range checks and tree agreement are not reliable uncertainty guarantees. Hard feature combinations can lie within every individual training range, and most trees can agree on a badly optimistic point.
- Severe expansions remain unsolved. The forest's labeled range ends at 17 workers; sparse costly examples are diluted by leaf averaging. Timing residuals extrapolate better on some moderate additions but miss the extreme cases too.
- The new seed-suffix bound is a controlled prototype only. No wider speed or accuracy claim is attached to it.

## Evaluation and engineering practices

- UNKNOWN means unresolved, not infeasible. Verified bills are upper bounds. Keep lower/upper intervals and censored cases in reports.
- Keep the original short-budget outcomes immutable when deeper searches improve physical evidence. A changed label is not an improved forecast.
- Negative marginal error means understated added labor or overstated savings. Report underestimates and overestimates separately, including threshold counts and expensive tails.
- A retrospective best-reference stopping time is unavailable to an online caller. Evaluate fixed-budget quality and implementable stopping rules separately.
- The solver's last-call timer is not complete compiler cost. Charge feature calculation, inference, policy choices, reuse/repair/fallback, launcher and output consistently; state whether independent verification is outside timing.
- Check the actual live process, not just a stored `running` status. A native assertion stopped one driver while its status file stayed running. Preserve the failed child and resume only unstarted work.
- Exact common inputs can have different bounded parallel solver outcomes. Record full query inputs and complete-course quality rather than attributing every difference to the estimator.
- Do not turn the 41-worker sentinel into a priced 40-worker prediction. Preserve missing probes, unsupported horizons, zero-action late hires and genuinely fixed commitments explicitly.
- Final-day phase 23 is not usable work time. Give the terminal reference its phase-22 deadlines before solving, and preserve earlier wrapper outcomes separately.
- Keep all five crops, three animals, additions, removals, layouts, seed/product/land releases, withdrawals, terminal days and unfamiliar calendars represented. A removal test is not an addition test in reverse.
- Export compact immutable C++ weights and reuse computed features. Avoid framework/process overhead in search. Measure total inference and decisions before considering quantization, SIMD or a heavier model.

The dated `docs/research/` reviews and two ledgers retain the full reasoning, failed attempts, corrections and evidence locations.
