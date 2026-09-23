# Design

## The boundary that made BC practical

The learned policy chooses *what the farm should accomplish today*. It never
chooses tiles, paths, shopping quantities, exact sale orders, or hour-by-hour
commands. Those decisions have hard rules and long mechanical dependencies, so
the deterministic day compiler owns them. This boundary lets BC learn strategy
from public replays without learning route legality or reproducing an expert's
incidental worker paths.

At every dawn the runtime performs this loop:

1. Encode the public observation, own private inventory, and only the agent's
   causal action history.
2. Greedily decode one valid `DayIntent` with the native FP32 network.
3. Bind the intent to the groups regenerated from the current observation.
4. Search for a funded day schedule under a 256-worker-attempt budget.
5. Execute it hour by hour, monitoring public changes and repairing when a
   route, market event, or funding assumption changes.
6. Record requested and executed commitment gaps for evaluation.

## Exact DayIntent contract

`day_compiler/source/intent.hpp` is authoritative. The contract hash is
`544d8ea871fa6ab44794f0c217395f7666e3adf6dfb1f8b255a6575d72940f67`.
It contains:

- Five counts of new wheat, carrot, melon, tomato, and strawberry crops.
- Three direct next-dawn population targets for geese, cows, and sheep.
- One boolean for buying the next land quadrant today.
- For every crop group, a partition of its members across reachable biological
  procedures. A procedure specifies a crop goal and water/fertilizer counts.
- For every animal group, how many members to serve today and how many may
  escape tonight.

Groups are semantic, not persistent IDs. The binder rebuilds crop and animal
groups from the dawn observation using biological state such as product,
age/phase, held output, water/fertilizer state, decay deadline, feeding/care,
and structure. Group members are interchangeable. The policy selects counts;
the compiler assigns concrete cells.

The binder fails fast on a non-dawn request, invalid or unused counts, a crop
or animal partition that does not sum to the group size, an unreachable crop
goal, impossible escape, impossible next-dawn animal targets, or a land request
when no land remains. It also masks decoder choices so most invalid requests
cannot be emitted.

## Day compiler

The selected compiler runs these stages:

1. **Bind and audit.** Convert group partitions to concrete biological
   commitments and start a consequence audit.
2. **Workload and resources.** Derive required crop/animal work, collections,
   returns, inputs, capacity, structures, land, and hires.
3. **Funding scenarios.** Forecast cash and market timing; choose purchases,
   ordinary and commitment-driven sales, and funding order positions.
4. **Worker search.** Assign tasks and solve routes against the exact engine.
   The production budget is 256 attempts, distributed over bounded candidate
   schedules.
5. **Verify.** Replay the complete proposed day in the engine. Reject schedules
   that are illegal, unfunded, physically incomplete, or violate intent.
6. **Refine.** Improve sales, wheat handling, and product-specific collections
   without sacrificing the verified commitments.
7. **React.** During the live day, compare the schedule with observations,
   monitor funding, and perform bounded repair or a legal emergency action.

The design was shaped by exact game rules and engine tests, not route imitation.
Failure reproductions added missing collection, financing, capacity, search
headroom, rival-order-position, and product-specific fallback logic. Fixed-task
dynamic-programming oracles separated solver mistakes from infeasible work.
Responsive full games exposed problems that replay-only checks could not.

## BC policy

The network has 509,359 parameters and uses two-layer MLP encoders/decoders at
width 128. It encodes global state, per-product state, crop groups, and animal
groups. Mean/max pooling makes group order irrelevant. Board tiles are excluded
from the selected model because the compiler owns physical placement.

The selected feature set adds cash/price/feed exposure, farm accounting,
cross-group coordination, available-space masks, and product-specific crop-plan
summaries. Causal history includes only actions already taken by this agent.

Decoding is structured:

- Crop procedure counts are autoregressive within each group and must exactly
  partition the group.
- Animal service counts are masked by group size and the terminal-day rule.
- Global decisions are decoded in the order land, animal populations, then new
  crops; earlier decisions condition and constrain later space use.
- Greedy temperature-zero decoding is used in production.

Training minimizes the sum of global, crop, and animal categorical negative log
likelihoods. Deployment uses the `BCW23004` native binary: named linear layers,
little-endian FP32 weights, no PyTorch dependency. C++ recreates the feature
encoder, masks, decoder order, and inference scratch space.

## Runtime failure handling

The agent first tries the raw intent. Recovery mode 1 can reduce the least
important growth commitment when compilation fails, then retries within the
same node budget. If no schedule survives, it emits a legal emergency action.
The selected evaluation had no uncompiled dawns or emergency actions in the
fresh compiler comparison, but these paths remain required for production.
