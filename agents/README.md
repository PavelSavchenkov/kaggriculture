# Agent storage

This is the authoritative C++ catalog of materially distinct agents found in
the repository. It includes the strongest validated local policy, the
teammate's strongest retained agent, current public controllers, top-player
replay phenotypes, and earlier in-house strategy families.

Agent packages are recursive:

- `common/` contains the local API, observation builder, generated build
  artifacts, and other shared inference support. It contains no agents.
- `inhouse/` contains selected policies whose complete strategic lineage was
  developed locally without opaque external behavior blocks.
- `external/` contains selected public notebook ports, teammate policies, and
  replay-derived policies. Each package records provenance, rating evidence,
  parity scope, optimization status, and restrictions.

There are no top-level agent aliases. The September 8 handoff explicitly retains
its historical league opponents so teammates can reproduce the comparisons.
Other generated parameter clones, ablations and broken candidates remain in
their self-contained experiments. Deprecated duplicates are stored recoverably
under `work/agent_storage_archive_20260901/`. They are not separate catalog
agents because they do not add a material phenotype or reliable strength
signal.

The September 8 references are:

- `external/animal_repair_q24_premium_m2`: latest submitted policy, official
  IDs 56101451 and identical repeat 56102764. Its frozen native audit beats the
  teammate in 4,040/4,096 games, but the broad native-margin promotion gate fails.
- `external/empty_sale_slots_m2`: accepted research reference.
- `external/cow_service_retained_q24_premium_m2`: discovery successor with cheaper
  cow schedules; unchanged wins and choices, not broadly promoted.
- `inhouse/early_melon_b98_m1`: independent cold reference; still weak.
- `external/teammate_shoprouter`: teammate's strongest retained benchmark.

Read [the complete handoff](../handoffs/sep08_agent/readme.md) for the 45-agent
league, lineage, exact gates, deployment rebuild and continuing optimization loop.
Earlier Bohann and investment agents remain useful historical references.

Discover official agents with:

```text
find agents/inhouse agents/external -mindepth 2 -maxdepth 2 -name agent.json
```

Every manifest declares only files inside its package. Generated binaries and
build directories are ignored and are not part of the catalog.
