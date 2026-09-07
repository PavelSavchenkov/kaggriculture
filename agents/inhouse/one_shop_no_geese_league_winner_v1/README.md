# One-shop no-geese league winner v1

This is the strongest retained in-house policy for the fully enabled game with
exactly the first random shop visible and with geese forbidden. It is a causal
child of `compiled_product_capacity_early_pressure_v1`, not an external replay
or a copied behavior block.

The strategy makes an observation-safe mixed-ruminant opening, turns early
surplus into liquidity without consuming reserved obligations, preserves
capacity at day boundaries, produces every required crop and animal product,
and closes its nominal sale plan on day 29. It never buys a goose or coop.

Against the frozen five-role league, the untouched 256-seed replication covers
20,480 games: all eight first-shop identities and both seats for every seed and
opponent. It scores `J_main = 0.95`, `J_worst = 0.75`, and mean cash margin
`+14,299.93`. Both seats score `0.95`. Every opponent has match points above
`0.5` and a positive mean cash margin. Candidate failures, discards, coops,
geese, and eggs are all zero.

Strict and native-LTO results are byte-identical. One-thread and four-thread
profiles are byte-identical. ASan and UBSan are clean. Reset reuse, self-play,
and one-, two-, and four-shop observation shapes pass. Measured action latency
is below 1 microsecond at p99 in both audited builds.

See `DESIGN_PROVENANCE.md` for the exact genome, lineage, controllers, compiler,
and artifact hashes. The complete discovery, replication, and correctness
audits are in
`experiments/v4/sep1_one_random_shop_no_geese_league/artifacts/`.
