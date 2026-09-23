# Existing worker backend

Snapshot of the persistent root day_policy source, with namespace separation.
Original source and snapshot hashes are in snapshots/worker_root_import.json.
No dependency on an older experiment. This component chooses worker movements.

Compiler-facing changes:
- Engine-sized worker buffers instead of a strategic 13-hire cap.
- Explicit total attempt and wall-time limits.
- Per-worker earliest hire hour and reserved order slots for funding sales.
- Harvest deadlines for semantic yield commitments.
- Execute and verify only the requested horizon; reject unavailable-hour work.
- Cumulative return requirements are minima, without an unrequested deposit cap.
- Current-hour starts with existing worker positions, ordered cargo and completed
  daily service. All action and deadline indices remain absolute day hours.
- Initial carried products return through tasks bound to their real owner.
- Re-evaluate an available suffix with replay_schedule before rebuilding it.

These are contract adaptations, not a new movement algorithm. Root historical
coverage and timings are not accepted as measurements of this fork. The worker
subproblem uses unlimited cash/capacity internally. The compiler must verify
funding, actual shed capacity and complete market orders in the full engine.
Use fixed returned actions during normal execution; do not reuse the embedded
worker Agent as a capacity-aware reactive controller.

The suffix extension reproduces all 2,304 saved dawn regression cases exactly.
Initial midpoint development fixtures rebuild 2,500/2,610 old and 1,313/1,345
fresh known-feasible suffixes with the unchanged 32-attempt budget. Replaying the
available original suffix verifies all 3,955, about 0.03 ms each. That last result
is an identity check; it does not measure recovery after a funding shock.
