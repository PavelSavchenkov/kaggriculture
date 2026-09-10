# Measurement protocol

Version 0 is the development protocol, frozen before the first candidate result. Separate confirmation manifests will fix exact families, seeds, weights, and budgets before opening their outcomes.

## Objective

Maximize final own cash minus final rival cash, averaged with fixed calendar-family and opponent-family weights. Compare policies on the same cases. Also report own cash, rival cash, win utility (win 1, tie 0.5, loss 0), worst-decile mean margin, obligation failures, and time.

Use verified baseline-feasible cases for paired quality comparisons. Retain all excluded and failed cases with reasons. Report newly funded plans separately. No silent survivor filtering, credit, phantom inventory, or retrospective reweighting.

## Iterations

Every iteration names its hypothesis, input manifest, baseline, candidate, deterministic work budget, measurement, and decision. Include setup/feature/scenario costs in end-to-end timing. Measure warm refinement and an independently built cold baseline. Fix implementation bottlenecks before scaling repeated work.

Use C++ for simulation, policy inference, and repeated search. Python may acquire data, generate offline fixtures, build, and summarize. Cache builds by source content; never compile or launch a process per game.

Review progress every 20 minutes and explicitly decide whether to reorder IDEAS.md or pivot. The original timed run ended September 10 at 05:00 UTC (06:00 London); freeze a new protocol for resumed work.

## Correctness gates

1. Exact engine agreement for per-unit market quotes, interleaving, affordability, inventory, seeds, animals, hire prices, land, consumption, and terminal timing in supported cases.
2. No new unmet obligations on certified baseline-feasible calendars.
3. Policy input excludes hidden evaluation futures and source identity.
4. Deterministic results for a fixed work budget; independent instances.
5. Full-game integration checks actual production, labor, purchases, storage and 719 transitions.

## Candidate promotion

Use grouped uncertainty estimates by source family and market world, not independent rows for crossed cases. Primary held-out panel has both unfamiliar calendars and unfamiliar market scenarios. Separate panels isolate each type of transfer.

- 95% lower confidence bound for mean margin gain above zero.
- 95% lower confidence bound for win-utility gain at least -0.0025.
- Predeclared critical groups: point mean margin gain at least zero, utility gain at least -0.0025, worst-decile margin change at least -$50.
- No unexplained new obligation or execution failures.
- Fixed, reported total compute budget with a useful throughput/quality tradeoff.

Threshold changes must be prospective and justified before examining confirmation results. Development results remain exposed evidence.

## Pipeline promotion

Integrate early with a fixed plan proposal set and unchanged day solver/estimator. Measure funded useful plans per CPU second and realized final value, not only financial replay score. Compare complete agents against reacting opponents in both seats. Native engine randomness and independent shop calendars are separate panels. A conditional component pass is insufficient for agent promotion.

## Public evidence

Refresh public replay/notebook metadata during the run. Store retrieval times, source URLs/IDs and hashes. Read notebook code as evidence; do not execute downloaded setup, upload or gameplay code. Ports must follow the local C++ agent API and retain source/license/parity scope. Reserve complete source families before analysis where possible.
# Scope across changing agent families

Use our agents and public agents as evidence sources. The target is a reusable financial component and general lessons about farm plans, not optimization for a permanently fixed agent family. Compare conditions such as input deadlines, deposit timing, cash, storage, market slots and rival output history across different implementations.

Record whether a finding is directly implied by game rules, observed in one family, supported by a controlled change, or reproduced across families and scenarios. Repeated versions of one program do not count as independent families. Keep both positive and negative evidence and the conditions under which a rule fails. A replay pattern does not reveal why its author chose it or prove that it is optimal.

Refresh strong public agents and notebooks during the run. Reserve new episodes before opening their action data when using them for confirmation. If a family or episode is opened for discovery, update the exposure record rather than continuing to call it unseen. Evaluate the same primitives on new strong plans as they become available, while retaining the previous strong schedules as baselines.

## Scope of the current evidence

The live course comparisons weight three supplied cow/sheep/goose courses, opponent packages, seats and shop panels equally within each frozen manifest. These courses share an implementation family, and several opponent packages are related. Seed-cluster intervals handle crossed game rows; they do not establish independent-family transfer. Public confirmation reserves whole new episodes before action inspection, but new names or submission IDs still do not prove independent source code.

`scripts/audit_component.py` checks the declared numerical gates for a completed live comparison, including every reported opponent, course and shop subgroup. It reports compute and the remaining judgment separately. Passing these component gates does not establish improved plan selection or a physically compatible rival scenario generator. Those are still required before pipeline promotion.
