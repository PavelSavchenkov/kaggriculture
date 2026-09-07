# public_deniz_v111_safe

Safety adaptation of the parity-proven Deniz V111 / Boatlee V16 source. It
requests the unchanged source action, reconstructs only the current public and
own-private simulator state, and removes action units that would fail against a
PASS opponent. It does not change the route, shop logic, or front-run timing.

The final implementation replaces the full sanitizer hot path with exact local
diagnostics and prefix reconstruction, while retaining the full engine
sanitizer for malformed or ambiguous input. It is action-identical over all
4,096 shop sequences and both seats and is 4.53x faster in the controlled
benchmark.

This remains lineage `boatlee_nikita_v16`; it is not an independent public
source or an independent phenotype vote from C68.
