# Lineage and selection

## Compiler lineage

The starting point was the frozen `sep22_day_compiler`: semantic dawn intent,
strict binder, workload/resource model, worker solver, complete engine replay,
and reactive execution. The Sep 23 lineage then fixed failures in this order:

1. escaping-animal collection and fertilizer collection;
2. partial compile recovery and value-driven collections;
3. collection-driven hires and ordinary-animal handling;
4. ongoing-crop care and shed-value accounting;
5. partial input sales, more search headroom, and collection deadlines;
6. funding that respects selected rival order positions;
7. product-specific collection fallback.

The last item produced compiler hash `ed154…` and is the compiler in this
folder. Against `teammate_shoprouter`, its fresh 64-game comparison won 52,
averaged +9,881.58 margin, and revised all 16 raw compile failures to zero final
failures. A later overflow-return patch looked better on a reused 32-game
development batch, but it had no matching fresh/local-LB evidence. It was not
promoted.

What shaped the compiler design:

- The requirement was to execute semantic intent well, not copy replay routes.
- Exact-engine replay, fixed-task oracles, and small failure repros identified
  mechanical defects faster than score alone.
- Full responsive opponents exposed collection timing, market order, financing,
  and repair failures hidden by static continuations.
- Deterministic node limits replaced machine-dependent search behavior.
- Strict causal observations prevented a good score from hiding information
  leakage.

## BC lineage

The corpus froze public top-20 replay perspectives and labeled dawns with
`DayIntent`, using the compiler contract. Earlier probes compared board input,
financial features, coordinated decoding, accounting, space masks, and product
plans. The selected parent uses all useful semantic features except board tiles.

Stage 1 trained across the complete rank-20 cohort: 49,860 training dawns and
10,320 validation dawns. Its best saved state was epoch 82 with validation loss
1.1406101. Stage 2 initialized from that state and fine-tuned only submission
56424800: 2,010 training dawns and 570 validation dawns. Epoch 21 was best at
0.85543248 validation loss. By epoch 500 validation loss had degraded to
2.04651034 despite continued training improvement, so `best.pt`, not the last
checkpoint, was exported.

The selected training source hashes are:

- Stage 1: `a6b1adf2982b66523ab539892552ed38d98272a073cb9bd6c4d6de6a9e055235`
- Stage 2: `0788fdadae7aed7976515899a1cfca39c04f9de394b42a80d8c5c5489106e78c`
- Dataset: `0ef20bdc02c99f3c157ad823b878f9e021dfb1d02df2494f9a6f941efbd0e45d`

The `compiler` fields in the two training contracts identify the compiler
snapshot present when each run was recorded (`44158…` for stage 1 and `30b776…`
for stage 2). They are provenance, not the release runtime. The later `ed154…`
compiler preserves the same DayIntent contract and was promoted only after the
fresh compiler and local-LB evaluations described here.

The fine-tuned Majkel-style policy was retained after broader architecture and
loss experiments because none had stronger complete fresh evidence. Sep 22 BC
results were excluded where data contamination could not be ruled out.

## Release decision

This release combines the epoch-21 model with compiler `ed154…`, ordinary
collection refinement enabled, recovery mode 1, and a 256-attempt budget. That
is the combination with completed native parity, compiler, latency, fresh-game,
and local-LB evidence. `provenance/selection.json` is the machine-readable
selection record.
