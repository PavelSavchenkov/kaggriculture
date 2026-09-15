# Day policy

Deterministic C++ day solver and tile placement. Use `../source/policy.hpp` and
`Solver::solve`. The default is Balanced-4 with Staged placement, hire cap 13,
and hire minimization enabled. Hires exclude the farmer. The independent native
verifier is `../source/verify.hpp`; the API follows `../SPEC.md`.

## Worker search

The compiler creates service visits and bounded shed-return jobs from event sets.
Routes use deterministic construction seeds, insertion search, reorderings,
relocations, swaps and tail exchanges. Fertilizer source/crop pairings are seeds;
search can form multi-animal collection and multi-crop service batches. Multiple
workers can share tile work, including ordered same-hour water/harvest actions.

Timed returns support partial deposits, several shed trips and work after return.
The executor never deposits unrequested products or exceeds final requested
quantities. It can move an unstarted crop during a repair. Every successful result
is replayed in the native engine and verified. Failure exposes no partial schedule
and does not prove that the input is impossible.

Profiles:

- Balanced-4 (default): `effort=Balanced`, `variants=4`. Limited extra timing,
  split-visit and finish-before-return candidates follow failed cheap attempts.
- Full-8: `effort=Full`, `variants=8`. Broader construction and repair search.
- Fast, Compact and Classic remain available for earlier frontier comparisons.

`route_rounds=1`, `minimize_variants=1` are ordinary settings. Balanced reuses its
successful construction when reducing hires, then tries at most two additional
cheap seeds. Full explores more alternatives. Calls stop at success for the
current hire count and keep a verified incumbent during minimization. No minimum
hire proof is claimed. Full is an independent call, not an automatic fallback.

## Placement

Staged placement uses only the current dawn and declared jobs:

- Place animals first in stable input order, near shed access and compatible
  housing. On an empty initial quadrant, prefer compact row/column arms.
- Give opening melons sites before wheat. When melons are declared, put wheat
  farther out, leaving middle sites available for later establishments.
- Softly reserve six near-shed sites per quadrant for animals. This never forbids
  a planting; a full crop plan can use every site.
- Strongly prefer crop sites cleared that day, preserving harvest/replant reuse.
  Rotate other crop placement across owned quadrants.
- Allow permanent animals to wait for suitable new land. Crops prefer usable
  land, especially when the purchase is at hour 16 or later.

Legacy placement is an explicit comparison option. Placement uses fixed arrays,
immutable geometry tables and a bounded scan of at most 100 sites per new job.
Days with no establishments skip placement work. No future replay data, player
identity or copied leader policy code enters either heuristic.

## Integration and limits

Tile timestamps are relative to dawn; see `policy.hpp`. Purchases and returns use
24 hour-indexed arrays. Returns are cumulative quantities delivered by workers,
excluding dawn shed stock. The caller handles affordability, market decisions,
shed capacity and night settlement. No fertilizer purchases or shed pickups are
supported. Hires occur at hours 0/1, with a hard maximum of 13.
Adding sales must respect remaining market-order slots and actual funding order;
the solver does not reserve extra slots for them.

Result state is after hour 23 actions and decay, before automatic night deposits
and random overnight changes. Propagate this actual state, including any repaired
crop placement, when planning the next day. The shortened terminal day is a
separate integration case.

The manifest's `Agent` is the low-level executor with a simple smoke-test day
planner. It is not a competitive complete decision engine. Use `Solver` for
supplied day plans; keep a separate instance per thread.

Fixed contiguous route buffers, compact cargo dimensions, cached travel bounds
and early abandonment of missed-deadline retries reduce cost. No mutable globals,
per-candidate heap allocation, manual SIMD or fast-math. Native O3/LTO is the
release build; prior optional PGO measurements are documented separately.

See `../measurements/REPORT.md` and `TESTING.md` for results and limits,
and `REPLAY_PATTERNS.md` for placement evidence. All runtime code is included.
