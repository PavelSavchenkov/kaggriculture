# Kaito H10 front-run fast

Same-lineage throughput port of the frozen Kaito H10 representative. It keeps
the unchanged public Kaito policy, both safety passes, the step-zero wheat
front-run, and the step-one repayment unchanged.

The implementation uses an exact sanitizer fast path, one reconstructed
simulator per decision, skips the redundant first safety shell after steps 0/1,
and uses a fixed terminal-sale array. A localized diagnostic must prove that
every requested action succeeds; otherwise the original sanitizer remains the
fallback. Exhaustive action parity covers all 4,096 shop sequences and both
seats. The final incremental end-to-end gain is about 11--16% on top of the
earlier 6.81x sanitizer speedup.
