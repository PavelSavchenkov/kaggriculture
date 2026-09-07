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

There are no top-level agent aliases. Generated search parameter clones,
ablation-only variants, broken candidates, and superseded checkpoints remain in
their self-contained experiments. Deprecated duplicates are stored recoverably
under `work/agent_storage_archive_20260901/`. They are not separate catalog
agents because they do not add a material phenotype or reliable strength
signal.

The two primary competitive references are:

- `external/investment_context_guarded_001_best`: strongest broadly validated
  local policy retained on September 7, 2026.
- `external/teammate_shoprouter`: C++ port of the teammate's strongest retained
  `shoprouter-rl-v2` agent.

Discover official agents with:

```text
find agents/inhouse agents/external -mindepth 2 -maxdepth 2 -name agent.json
```

Every manifest declares only files inside its package. Generated binaries and
build directories are ignored and are not part of the catalog.
