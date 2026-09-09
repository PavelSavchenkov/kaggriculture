# Goose course with the correct parent context

Candidate: ../late_animal_schedule_001/proposals/late_goose_wheat_context.
It preserves the existing day12 tomato-shop decision before replacing the
intended wheat suffix with the optimized goose course. This fixes a real
integration error: physical state equality alone did not identify which future
composition the parent had selected.

CONTEXT_CAUSAL.json uses the previous1790000 cohort diagnostically. All181
unintended tomato replacements disappear; the intended goose games keep their
gains. No threshold was fitted: the gate uses the parent's existing two-tomato-
shop rule. The complete implementation passes896 generic/pair/debug/thread
and self/PASS checks, including native and custom shop streams.

Fresh1810000..1810511, both seats,21opponents and two policies:43008games.
Against the parent directly:420wins,480ties,124losses, utility.64453125,
mean margin+$80.35449. Every opponent's paired mean margin improves.

The policy is nevertheless NOT PROMOTED. Current grouped utility falls
.9454833984 to.9407836914; paired95% interval is[-.00769043,-.00201416].
Historical noninferiority and individual win-rate limits also fail. Some small
negative investment returns turn earlier near-ties/wins into losses. Keeping
the correct composition context fixes the large failures but does not replace
an economic rule for choosing goose, cow, sheep or retaining crops.

FRESH_ANALYSIS.json retains all21 opponents, means, utility and tails;
FRESH_PREREGISTERED.json records the frozen candidate and gates. The prior
opening_q32_b13_v1 remains the promoted experimental reference. Neither this
candidate nor its predecessor was submitted or pushed.

Next use the existing daily whole-farm market estimator on the compiled
buy/sell plans, with actual achievable hire costs and fixed seed/animal costs.
Include optimized cow/sheep routes from the preceding run. Compare estimates
with this now-used panel for diagnosis, then use a new untouched seed range
for any fitted selector. Do not reuse1810000 as a fresh validation claim.
