# Boatlee H7 fast sanitizer

Same-lineage throughput copy of the selected Boatlee H7 policy. It retains the
unchanged V21-R1 source policy, H7 active-safety order, and capacity guard. Its
sanitizer returns an already-valid action after the engine's localized exact
diagnostic and falls back to the original parity-hash sanitizer on any failed
or ambiguous request.

The policy has no searched parameters and makes no economic change. The final
implementation also replaces bounded vectors with fixed arrays, reuses one
reconstructed simulator, caches clone-distance work, and skips capacity
simulation when a conservative bound proves it unnecessary. Exact action
parity covers all 4,096 shop sequences and both seats; the final incremental
speedup is 1.03x on top of the earlier 13.04x sanitizer speedup.
