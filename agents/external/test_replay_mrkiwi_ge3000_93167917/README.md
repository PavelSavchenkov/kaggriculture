# test_replay_mrkiwi_ge3000_93167917

Exact action-tape reconstruction of MrKiwi in public episode 93167917 (ge3000, engine 1.32.7). The tape is the unchanged recorded action sequence; it has no weed repair or market fallback. All variants from this player remain in lineage `replay_mrkiwi`. The reserved validation replay is used only for phenotype-fidelity checks.

On a trajectory with a different worker count, the runtime emits the taped
prefix for workers that exist and `PASS` for any extra workers. This is the
minimum API adaptation needed to keep the action structurally valid; all taped
market requests remain unchanged.
